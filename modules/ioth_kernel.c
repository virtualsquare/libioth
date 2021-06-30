/*
 *   libioth: choose your networking library as a plugin at run time.
 *   plugin for the kernel stack
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

#include <ioth.h>

void *ioth_kernel_newstack(const char *vnlv[], const char *options,
		struct ioth_functions *ioth_f) {
	(void) vnlv;
	(void) options;
	ioth_f->socket = socket;
	ioth_f->close = close;
	ioth_f->bind = bind;
	ioth_f->connect = connect;
	ioth_f->listen = listen;
	ioth_f->accept = accept;
	ioth_f->getsockname = getsockname;
	ioth_f->getpeername = getpeername;
	ioth_f->setsockopt = setsockopt;
	ioth_f->getsockopt = getsockopt;
	ioth_f->shutdown = shutdown;
	ioth_f->ioctl = ioctl;
	ioth_f->fcntl = fcntl;
	ioth_f->read = read;
	ioth_f->readv = readv;
	ioth_f->recv = recv;
	ioth_f->recvfrom = recvfrom;
	ioth_f->recvmsg = recvmsg;
	ioth_f->write = write;
	ioth_f->writev = writev;
	ioth_f->send = send;
	ioth_f->sendto = sendto;
	ioth_f->sendmsg = sendmsg;
	return (void *) 42; // useless, but retval == NULL means error!
}

void *ioth_kernel_n_newstack(const char *vnlv[], const char *options,
		struct ioth_functions *ioth_f)
	__attribute__ ((alias ("ioth_kernel_newstack")));
