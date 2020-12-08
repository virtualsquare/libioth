/*
 *   libioth: choose your networking library as a plugin at run time.
 *   plugin for picoxnet stack (picox runs entirely in user space)
 *
 *   Copyright (C) 2020  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
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

#include <libioth.h>
#include <picoxnet.h>
#include <stdarg.h>
NLINLINE_LIBMULTI(picox_)

static typeof(getstackdata_prototype) *getstackdata;

static int picox_iplink_ifadd(struct picox *stackdata, const char *ifvnl) {
	char *delim = strstr(ifvnl, "://");  // position of "://"
	char *colonmark = strchr(ifvnl, ':'); // position of "="
	/* if the patter is "ifname=vnl" i.e. "...=...://..." i
		 or "...=..." (without "://" */
	if (colonmark && (!delim  || (delim && colonmark < delim))) {
		/* spit ifname from vnl */
		int ifnamelen = colonmark - ifvnl;
		char ifname[ifnamelen + 1];
		const char *ifacevnl = colonmark + 1;
		snprintf(ifname, ifnamelen + 1, "%.*s", ifnamelen, ifvnl);
		picox_iplink_add(stackdata, ifname, -1, "vde", ifacevnl);
	} else
		/* no if name */
		picox_iplink_add(stackdata, NULL, -1, "vde", ifvnl);
}

void *ioth_picox_newstack(const char *vnlv[], struct ioth_functions *ioth_f) {
	struct picox *stackdata = picox_newstack(NULL);
	if (vnlv != NULL) {
		int i;
		for (i = 0; vnlv[i] != NULL; i++)
			picox_iplink_ifadd(stackdata, vnlv[i]);
	}
	getstackdata = ioth_f->getstackdata;
	ioth_f->close = picox_close;
	ioth_f->bind = picox_bind;
	ioth_f->connect = picox_connect;
	ioth_f->listen = picox_listen;
	ioth_f->accept = picox_accept;
	ioth_f->getsockname = picox_getsockname;
	ioth_f->getpeername = picox_getpeername;
	ioth_f->setsockopt = picox_setsockopt;
	ioth_f->getsockopt = picox_getsockopt;
	ioth_f->shutdown = picox_shutdown;
	ioth_f->read = picox_read;
	ioth_f->readv = picox_readv;
	ioth_f->recv = picox_recv;
	ioth_f->recvfrom = picox_recvfrom;
	ioth_f->recvmsg = picox_recvmsg;
	ioth_f->write = picox_write;
	ioth_f->writev = picox_writev;
	ioth_f->send = picox_send;
	ioth_f->sendto = picox_sendto;
	ioth_f->sendmsg = picox_sendmsg;
	return stackdata;
}

int ioth_picox_delstack(void *stackdata) {
	picox_delstack((struct picox *) stackdata);
	return 0;
}

int ioth_picox_socket(int domain, int type, int protocol) {
	struct picox *stackdata = getstackdata();
	return picox_msocket(stackdata, domain, type, protocol);
}

int ioth_picox_ioctl(int fd, unsigned long request, ...) {
	va_list ap;
	va_start(ap, request);
	void *arg = va_arg(ap, void *);
	va_end(ap);
	return picox_ioctl(fd, request, arg);
}

int ioth_picox_fcntl(int fd, int cmd, ...) {
	va_list ap;
	va_start(ap, cmd);
	long arg = va_arg(ap, long);
	va_end(ap);
	return picox_fcntl(fd, cmd, arg);
}

void *ioth_picox_n_newstack(const char *vnlv[], struct ioth_functions *ioth_f)
  __attribute__ ((alias ("ioth_picox_newstack")));
int ioth_picox_n_delstack(void *stackdata)
  __attribute__ ((alias ("ioth_picox_delstack")));
int ioth_picox_n_socket(int domain, int type, int protocol)
  __attribute__ ((alias ("ioth_picox_socket")));
int ioth_picox_n_ioctl(int fd, unsigned long request, ...)
  __attribute__ ((alias ("ioth_picox_ioctl")));
int ioth_picox_n_fcntl(int fd, int cmd, ...)
  __attribute__ ((alias ("ioth_picox_fcntl")));
