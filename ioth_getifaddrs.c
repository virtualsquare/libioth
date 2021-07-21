/*
 *   libioth: choose your networking library as a plugin at run time.
 *   ioth_getifaddrs: getifaddrs(3)/freeifaddrs(3) replacements for ioth
 *
 *   Copyright (C) 2021  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
 *
 *   This library is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation; either version 2.1 of the License, or (at
 *   your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_arp.h>

#include <ioth.h>

#define FORALL_NLMSG(nlmsg, buf, len) \
	for(nlmsg = (struct nlmsghdr *) buf; NLMSG_OK (nlmsg, len); nlmsg = NLMSG_NEXT (nlmsg, len))

#define NLA_OK(nla, len) ((len) >= (int)sizeof(struct nlattr) && \
		((nla)->nla_len & NLA_TYPE_MASK) >= sizeof(struct nlattr) && \
		((nla)->nla_len & NLA_TYPE_MASK) <= (len))

#define NLA_NEXT(nla,attrlen) ((attrlen) -= RTA_ALIGN(((nla)->nla_len & NLA_TYPE_MASK)), \
		(struct nlattr*)(((char*)(nla)) + RTA_ALIGN(((nla)->nla_len & NLA_TYPE_MASK))))

/* scan netlink attributes -> load pointers in the attrs array */
static void nl_getattrs(void *attrbase, size_t attrlen, struct nlattr **attrs, int nattrs) {
	struct nlattr *scan;
	memset(attrs, 0, nattrs * sizeof(struct attr *));
	for (scan = attrbase; NLA_OK(scan, attrlen); scan = NLA_NEXT(scan, attrlen)) {
		if (scan->nla_type < nattrs)
			attrs[scan->nla_type] = scan;
	}
}

/* delete a struct ifaddrs element */
#define ck_n_free(x) if (x) free(x)
static void _freeifaddr(struct ifaddrs *ifa) {
	ck_n_free(ifa->ifa_name);
	ck_n_free(ifa->ifa_addr);
	ck_n_free(ifa->ifa_netmask);
	ck_n_free(ifa->ifa_broadaddr);
	ck_n_free(ifa->ifa_data);
	free(ifa);
}

/* add an element at the end of the ifap list */
static void add_ifaddrs(struct ifaddrs *new, struct ifaddrs **ifap) {
	new->ifa_next = NULL;
	for (; *ifap != NULL; ifap = &((*ifap)->ifa_next))
		;
	*ifap = new;
}

/******************** first netlink request (GETLINK/AF_PACKET) */
/* add name */
static char *add_ifla_ifname(struct nlattr *attr) {
	if (attr)
		return strndup((char *)(attr + 1), attr->nla_len);
	return NULL;
}

/* add a sockaddr_ll (ifa_addr, ifa_broadaddr) */
static void *add_sockaddr_ll(struct nlattr *attr, int ifindex, unsigned ifi_type) {
	struct sockaddr_ll *ll;
	if (attr != NULL && attr->nla_len > sizeof(*attr) &&
			attr->nla_len <= sizeof(*attr) + sizeof(ll->sll_addr)) {
		int halen = attr->nla_len - sizeof(*attr);
		ll = calloc(1, sizeof(*ll));
		if (ll) {
			ll->sll_family = AF_PACKET;
			ll->sll_ifindex = ifindex;
			ll->sll_hatype = ifi_type;
			ll->sll_halen = halen;
			memcpy (ll->sll_addr, attr + 1, halen);
			return ll;
		}
	}
	return NULL;
}

/* add ifa_data from IFLA_STATS */
static void *add_ifla_data(struct nlattr *attr) {
	if (attr != NULL && attr->nla_len == sizeof(*attr) + sizeof(struct rtnl_link_stats)) {
		void *data = malloc(sizeof(struct rtnl_link_stats));
		if (data) {
			memcpy(data, attr + 1, sizeof(struct rtnl_link_stats));
			return data;
		}
	}
	return NULL;
}

/* process a GETLINK reply item -> create an AF_PACKET ifaddrs elemet */
static void add_af_packet(struct nlmsghdr *nlmsg, struct ifaddrs **ifap) {
	struct ifinfomsg *info = (struct ifinfomsg *) (nlmsg + 1);
	int32_t len = nlmsg->nlmsg_len - sizeof(*info);
	if (len < 0)
		return;
	struct ifaddrs *new = calloc(1, sizeof(*new));
	if (new) {
		struct nlattr *attrs[IFLA_MAX + 1];
		nl_getattrs(info + 1, len - sizeof(*info), attrs, IFLA_MAX + 1);
		if ((new->ifa_name = add_ifla_ifname(attrs[IFLA_IFNAME])) == NULL)
			goto err;
		new->ifa_flags = info->ifi_flags;
		if ((new->ifa_addr = add_sockaddr_ll(attrs[IFLA_ADDRESS], info->ifi_index, info->ifi_type)) == NULL)
			goto err;
		new->ifa_broadaddr = add_sockaddr_ll(attrs[IFLA_BROADCAST], info->ifi_index, info->ifi_type);
		new->ifa_data = add_ifla_data(attrs[IFLA_STATS]);
		add_ifaddrs(new, ifap);
	}
	return;
err:
	_freeifaddr(new);
}

/******************** second netlink request (GETADDR/AF_INET{6}) */
/* add a sockaddr_in/AF_INET field (ifa_addr, ifa_broadaddr, ifa_dstaddr)*/
static void *add_sockaddr_in(struct nlattr *attr) {
	if (attr != NULL && attr->nla_len == sizeof(*attr) + sizeof(struct in_addr)) {
		struct sockaddr_in *in = calloc(1, sizeof(*in));
		if (in) {
			in->sin_family = AF_INET;
			memcpy(&in->sin_addr, attr + 1, sizeof(struct in_addr));
			return in;
		}
	}
	return NULL;
}

/* add a sockaddr_in6/AF_INET6 field (ifa_addr, ifa_broadaddr, ifa_dstaddr)*/
static void *add_sockaddr_in6(struct nlattr *attr, uint32_t scope_id) {
	if (attr != NULL && attr->nla_len == sizeof(*attr) + sizeof(struct in6_addr)) {
		struct sockaddr_in6 *in = calloc(1, sizeof(*in));
		if (in) {
			in->sin6_family = AF_INET6;
			if (IN6_IS_ADDR_LINKLOCAL(attr + 1) || IN6_IS_ADDR_MC_LINKLOCAL(attr + 1))
				in->sin6_scope_id = scope_id;
			memcpy(&in->sin6_addr, attr + 1, sizeof(struct in6_addr));
			return in;
		}
	}
	return NULL;
}

/* convert IPv4 prefix to add_sockaddr_in (ifa_netmask) */
static void *add_sockaddr_netmask(unsigned prefixlen) {
	struct sockaddr_in *in = calloc(1, sizeof(*in));
	if (in) {
		uint32_t mask = (~0U) << (32 - prefixlen);
		in->sin_family = AF_INET;
		in->sin_addr.s_addr = htonl(mask);
		return in;
	}
	return NULL;
}

/* convert IPv6 prefix to add_sockaddr_in6 (ifa_netmask) */
static void *add_sockaddr_netmask6(unsigned prefixlen) {
	struct sockaddr_in6 *in = calloc(1, sizeof(*in));
	if (in) {
		in->sin6_family = AF_INET6;
		for (int i = 0; i < 16 && prefixlen > 0; i++, prefixlen -= 8) {
			if (prefixlen > 7)
				in->sin6_addr.s6_addr[i] = 0xffu;
			else
				in->sin6_addr.s6_addr[i] = 0xffu << (8 - prefixlen);
		}
		return in;
	}
	return NULL;
}

/* search the AF_PACKET element, the key is if_index. Some data must be copied to AF_INET{6} fields */
static struct ifaddrs *search_iface(int index, struct ifaddrs *scan) {
	for (; scan != NULL; scan = scan->ifa_next) {
		struct sockaddr_ll *addr_ll = (void *) scan->ifa_addr;
		if (index == addr_ll->sll_ifindex)
			return scan;
	}
	return NULL;
}

/* process a GETADDR reply item -> create an AF_INET/AF_INET6 ifaddrs elemet */
static void add_af_inet(struct nlmsghdr *nlmsg, struct ifaddrs **ifap) {
	struct ifaddrmsg *info = (struct ifaddrmsg *) (nlmsg + 1);
	int32_t len = nlmsg->nlmsg_len - sizeof(*info);
	if (len < 0)
		return;
	switch (info->ifa_family) { // select supported AF only
		case AF_INET: break;
		case AF_INET6: break;
		default: return;
	}
	struct ifaddrs *ifa_iface = search_iface(info->ifa_index, *ifap);
	if (ifa_iface == NULL)
		return;
	struct ifaddrs *new = calloc(1, sizeof(*new));
	struct nlattr *attrs[IFA_MAX + 1];
	nl_getattrs(info + 1, len, attrs, IFA_MAX + 1);
	new->ifa_name = strdup(ifa_iface->ifa_name);
	new->ifa_flags = ifa_iface->ifa_flags;

	switch (info->ifa_family) {
		case AF_INET:
			{
				if (ifa_iface->ifa_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) {
					if ((new->ifa_addr = add_sockaddr_in(attrs[IFA_LOCAL])) == NULL)
						goto err;
					new->ifa_dstaddr = add_sockaddr_in(attrs[IFA_ADDRESS]);
				} else {
					if ((new->ifa_addr = add_sockaddr_in(attrs[IFA_ADDRESS])) == NULL)
						goto err;
					new->ifa_broadaddr = add_sockaddr_in(attrs[IFA_BROADCAST]);
				}
				if ((new->ifa_netmask = add_sockaddr_netmask(info->ifa_prefixlen)) == NULL)
					goto err;
			}
			break;
		case AF_INET6:
			{
				if (ifa_iface->ifa_flags & IFF_POINTOPOINT) {
					if ((new->ifa_addr = add_sockaddr_in6(attrs[IFA_LOCAL], info->ifa_index)) == NULL)
						goto err;
					new->ifa_dstaddr = add_sockaddr_in6(attrs[IFA_ADDRESS], info->ifa_index);
				} else {
					if ((new->ifa_addr = add_sockaddr_in6(attrs[IFA_ADDRESS], info->ifa_index)) == NULL)
						goto err;
					new->ifa_broadaddr = add_sockaddr_in6(attrs[IFA_BROADCAST], info->ifa_index);
				}
				if ((new->ifa_netmask = add_sockaddr_netmask6(info->ifa_prefixlen)) == NULL)
					goto err;
			}	
			break;
	}
	add_ifaddrs(new, ifap);
	return;
err:
	_freeifaddr(new);
}

typedef void add_af_generic(struct nlmsghdr *nlmsg, struct ifaddrs **ifap);

/* ioth_getifaddrs, see getifaddrs(3) */
int ioth_getifaddrs(struct ioth *stack, struct ifaddrs **ifap) {
	struct sockaddr_nl sanl = {AF_NETLINK, 0, 0, 0};
	/* pre-baked netlink request packets */
	struct {
		struct nlmsghdr h;
		struct ifinfomsg i;
	} ifquery = {
		.h.nlmsg_len = sizeof(ifquery),
		.h.nlmsg_type = RTM_GETLINK,
		.h.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP,
		.h.nlmsg_seq = 1,
	};
	struct {
		struct nlmsghdr h;
		struct ifaddrmsg i;
	} adquery = {
		.h.nlmsg_len = sizeof(adquery),
		.h.nlmsg_type = RTM_GETADDR,
		.h.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP,
		.h.nlmsg_seq = 2,
	};
	struct nlmsghdr *query[] = {&ifquery.h, &adquery.h};
	add_af_generic *add_af[] = {add_af_packet, add_af_inet};

#define NQR (sizeof(query)/sizeof(query[0]))
	int fd;
	ssize_t replylen;
	*ifap = NULL;

	if ((fd  = ioth_msocket(stack, AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE)) < 0)
		return -1;
	if (ioth_bind(fd, (struct sockaddr *) &sanl, sizeof(struct sockaddr_nl)) < 0)
		goto err;

	// first request: GETLINK
	// second request: GETADDR
	for (int nq = 0; nq < 2; nq++) {
		if (ioth_send(fd, query[nq], query[nq]->nlmsg_len, 0) < 0)
			goto err;
		while (1) {
			if ((replylen = ioth_recv(fd, NULL, 0, MSG_PEEK|MSG_TRUNC)) < 0)
				replylen = 16384;
			unsigned char replybuf[replylen];
			if ((replylen = ioth_recv(fd, replybuf, replylen, 0)) < 0)
				goto err;
			struct nlmsghdr *nlmsg;
			FORALL_NLMSG(nlmsg, replybuf, replylen) {
				if (nlmsg->nlmsg_type == NLMSG_DONE)
					break;
				if (nlmsg->nlmsg_type == NLMSG_ERROR) {
					struct nlmsgerr *nlerr = (struct nlmsgerr *)(nlmsg + 1);
					if (nlmsg->nlmsg_len < NLMSG_LENGTH (sizeof (*nlerr)))
						errno = EIO;
					else
						errno = -nlerr->error;
					goto err;
				}
				add_af[nq](nlmsg, ifap);
			}
			if (nlmsg->nlmsg_type == NLMSG_DONE)
				break;
		}
	}
	close(fd);
	return 0;
err:
	close(fd);
	ioth_freeifaddrs(*ifap);
	*ifap = NULL;
	return -1;
}

/* ioth_freeifaddrs:  see freeifaddrs(3) */
void ioth_freeifaddrs(struct ifaddrs *ifa) {
	while (ifa != NULL) {
		struct ifaddrs *next = ifa->ifa_next;
		_freeifaddr(ifa);
		ifa = next;
	}
}
