#ifndef LIBIOTH_H
#define LIBIOTH_H
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>

#include <nlinline+.h>

struct ioth;

/* ioth stack create/destroy */
/* License Management: set program license (SPDX Specification) */
void ioth_set_license(const char *license);

/* one interface (or no interfaces if vnl == NULL) */
struct ioth *ioth_newstack(const char *stack, const char *vnl);

/* up to several interfaces */
struct ioth *ioth_newstackl(const char *stack, const char *vnl, ... /* (char  *) NULL */);
struct ioth *ioth_newstackv(const char *stack, const char *vnlv[]);
int ioth_delstack(struct ioth *iothstack);

void ioth_set_defstack(struct ioth *iothstack);
struct ioth *ioth_get_defstack(void);

/* communication primitives */
int ioth_msocket(struct ioth *iothstack, int domain, int type, int protocol);
int ioth_socket(int domain, int type, int protocol);
int ioth_close(int fd);

int ioth_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);
int ioth_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);
int ioth_listen(int fd, int backlog);
int ioth_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);
int ioth_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);
int ioth_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t ioth_read(int fd, void *buf, size_t len);
ssize_t ioth_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t ioth_recv(int fd, void *buf, size_t len, int flags);
ssize_t ioth_recvfrom(int fd, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen);
ssize_t ioth_recvmsg(int fd, struct msghdr *msg, int flags);
ssize_t ioth_write(int fd, const void *buf, size_t len);
ssize_t ioth_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t ioth_send(int fd, const void *buf, size_t len, int flags);
ssize_t ioth_sendto(int fd, const void *buf, size_t len, int flags,
		const struct sockaddr *to, socklen_t tolen);
ssize_t ioth_sendmsg(int fd, const struct msghdr *msg, int flags);
int ioth_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int ioth_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);
int ioth_shutdown(int fd, int how);
int ioth_ioctl(int fd, unsigned long cmd, void *argp);
int ioth_fcntl(int fd, int cmd, long val);

NLINLINE_LIBMULTI(ioth_)

	int ioth_getifaddrs(struct ioth *stack, struct ifaddrs **ifap);
	void ioth_freeifaddrs(struct ifaddrs *ifa);

	/* ----------------------------------- for ioth plugins */

	struct ioth_functions;
	void *newstack_prototype(const char *vnlv[], const char *options,
			struct ioth_functions *ioth_f);
int delstack_prototype(void *stackdata);
void *getstackdata_prototype(void);

/* libc + _GNU_SOURCE uses a transparent union for sockaddr
 * (__SOCKADDR_ARG __CONST_SOCKADDR_ARG)
 * Unfortunately this choice generates warnings for gcc in pedantic mode.
 * so ioth_X is used instead of X as protoype for all X in Berkeley
 * sockets API using sockaddr.
 */

struct ioth_functions {
	typeof(getstackdata_prototype) *getstackdata;
	typeof(newstack_prototype) *newstack;
	typeof(delstack_prototype) *delstack;
	typeof(socket) *socket;
	typeof(close) *close;
	typeof(ioth_bind) *bind;
	typeof(ioth_connect) *connect;
	typeof(listen) *listen;
	typeof(ioth_accept) *accept;
	typeof(ioth_getsockname) *getsockname;
	typeof(ioth_getpeername) *getpeername;
	typeof(setsockopt) *setsockopt;
	typeof(getsockopt) *getsockopt;
	typeof(shutdown) *shutdown;
	typeof(ioctl) *ioctl;
	typeof(fcntl) *fcntl;
	typeof(read) *read;
	typeof(readv) *readv;
	typeof(recv) *recv;
	typeof(ioth_recvfrom) *recvfrom;
	typeof(recvmsg) *recvmsg;
	typeof(write) *write;
	typeof(writev) *writev;
	typeof(send) *send;
	typeof(ioth_sendto) *sendto;
	typeof(sendmsg) *sendmsg;
};

/* ------------------ MAC address conversions --------------- */

#define MAC_ADDRSTRLEN 18

/* This  function converts a 6 byte MAC address family into a character string.
	 The buffer dst must be at least MAC_ADDRSTRLEN bytes */
static inline const char *ioth_ntomac(const void *src, char *dst, size_t size) {
	const unsigned char *mac = src;
	snprintf(dst, size, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return dst;
}

/* This  function converts the character string src into a 6 byte MAC address.
	 src points to a characteer string in the form "xx:xx:xx:xx:xx:xx" or
	 "xx-xx-xx-xx-xx-xx" where xx is a sequence of two hexadecimal digits
	 (not case sensitive) */
static inline int ioth_macton(const char *src, void *dst) {
	unsigned char *mac = dst;
	if (sscanf(src, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				mac, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5) != 6)
		if (sscanf(src, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
					mac, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5) != 6)
			return 0;
	return 1;
}

#endif
