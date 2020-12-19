/*
 *   libioth: choose your networking library as a plugin at run time.
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

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>
#include <config.h>

#include <fduserdata.h>
#include <ioth.h>

static FDUSERDATA *fdtable;
static __thread void *stackdata;

#define FOREACHDEFFUN \
	__MACROFUN(newstack) \
	__MACROFUN(delstack) \
	FOREACHFUN
#define FOREACHFUN \
	__MACROFUN(socket) \
	__MACROFUN(close) \
	__MACROFUN(bind) \
	__MACROFUN(connect) \
	__MACROFUN(listen) \
	__MACROFUN(accept) \
	__MACROFUN(getsockname) \
	__MACROFUN(getpeername) \
	__MACROFUN(setsockopt) \
	__MACROFUN(getsockopt) \
	__MACROFUN(shutdown) \
	__MACROFUN(ioctl) \
	__MACROFUN(fcntl) \
	__MACROFUN(read) \
	__MACROFUN(readv) \
	__MACROFUN(recv) \
	__MACROFUN(recvfrom) \
	__MACROFUN(recvmsg) \
	__MACROFUN(write) \
	__MACROFUN(writev) \
	__MACROFUN(send) \
	__MACROFUN(sendto) \
	__MACROFUN(sendmsg)

struct ioth {
	void *handle;
	void *stackdata;
	_Atomic unsigned int count;
	struct ioth_functions f;
};

static struct ioth default_iothstack = {
#define __MACROFUN(X) .f.X = X,
	FOREACHFUN
#undef __MACROFUN
};

static void *getstackdata(void) {
	return stackdata;
}

#define SYMBOL_PREFIX "ioth_"
#ifndef USER_IOTH_PATH
#define USER_IOTH_PATH "/.ioth"
#endif
#ifndef SYSTEM_IOTH_PATH
#define SYSTEM_IOTH_PATH "/usr/local/lib/ioth"
#endif

static inline char *gethomedir(void) {
	char *homedir = getenv("HOME");
	/* If there is no home directory, use CWD */
	if (!homedir)
		homedir = ".";
	return homedir;
}

static void *ioth_dlopen(const char *modname, int flags) {
	char path[PATH_MAX];
	char *homedir = gethomedir();

#define TRY_DLOPEN(LMID, ...) \
	do { \
		void *handle; \
		snprintf(path, PATH_MAX, __VA_ARGS__); \
		if ((handle = dlmopen(LMID, path, flags))) { \
			return handle; \
		} \
	} while(0)
	TRY_DLOPEN(LM_ID_BASE, "%s%s/ioth_%s-r.so", homedir, USER_IOTH_PATH, modname);
	TRY_DLOPEN(LM_ID_NEWLM, "%s%s/ioth_%s.so", homedir, USER_IOTH_PATH, modname);
	TRY_DLOPEN(LM_ID_BASE, "%s/ioth_%s-r.so", SYSTEM_IOTH_PATH, modname);
	TRY_DLOPEN(LM_ID_NEWLM, "%s/ioth_%s.so", SYSTEM_IOTH_PATH, modname);
#ifdef DEBUG
	TRY_DLOPEN(LM_ID_BASE, "./ioth_%s-r.so", modname);
	TRY_DLOPEN(LM_ID_NEWLM, "./ioth_%s.so", modname);
#endif
#undef TRY_DLOPEN
	return NULL;
}

static void *ioth_dlsym(void *handle, const char *modname, const char *symbol) {
	size_t extended_symbol_len = sizeof(SYMBOL_PREFIX) + 1 + strlen(modname) + strlen(symbol);
	char extended_symbol[extended_symbol_len];
	snprintf(extended_symbol, extended_symbol_len, SYMBOL_PREFIX "%s_%s", modname, symbol);
	// printf("%s\n", extended_symbol);
	return dlsym(handle, extended_symbol);
}

#define gotoerr(err, label) do {errno = err; goto label;} while(0)

static struct ioth *_ioth_newstackv(const char *stack, const char *options, const char *vnlv[]) {
	struct ioth *iothstack = calloc(1, sizeof(struct ioth));
	if (iothstack == NULL)
		gotoerr (ENOMEM, retNULL);
	if (stack == NULL || *stack == '\0') {
		*iothstack = default_iothstack;
		iothstack->count = 0;
	} else {
		iothstack->handle = ioth_dlopen(stack, RTLD_NOW);
		// printf("dlopen %p\n", iothstack->handle);
		if (iothstack->handle == NULL)
			gotoerr (ENOTSUP, errdl);
		iothstack->f.getstackdata = getstackdata;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#define __MACROFUN(X) iothstack->f.X = ioth_dlsym(iothstack->handle, stack, #X);
		{ FOREACHDEFFUN }
#undef __MACROFUN
#pragma GCC diagnostic pop
		if (iothstack->f.newstack == NULL)
			gotoerr (ENOENT, errnoioth);
		iothstack->stackdata = iothstack->f.newstack(vnlv, options, &iothstack->f);
		if (iothstack->stackdata == NULL)
			goto errnoioth;
	}
	return iothstack;
errnoioth:
	dlclose(iothstack->handle);
errdl:
	free(iothstack);
retNULL:
	return NULL;
}

struct ioth *ioth_newstackv(const char *stack, const char *vnlv[]) {
	char *options;
	if (stack == NULL || (options = strchr(stack, ',')) == NULL)
		return _ioth_newstackv(stack, "", vnlv);
	else {
		char stacklen = (++options) - stack;
		char _stack[stacklen];
		snprintf(_stack, stacklen, "%s", stack);
		return _ioth_newstackv(_stack, options, vnlv);
	}
}

int ioth_delstack(struct ioth *iothstack) {
	int retval;
	if (iothstack == NULL)
		return errno = EINVAL, -1;
	if (iothstack->count > 0)
		return errno = EBUSY, -1;
	if (iothstack->f.delstack == NULL)
		retval = 0;
	else
		retval = iothstack->f.delstack(iothstack->stackdata);
	if (retval == 0) {
		if (iothstack->handle != NULL)
			dlclose(iothstack->handle);
		free(iothstack);
	}
	return retval;
}

struct ioth *ioth_newstack(const char *stack) {
	const char *vnlv[] = {(char *) NULL};
	return ioth_newstackv(stack, vnlv);
}

struct ioth *ioth_newstacki(const char *stack, const char *vnl) {
	const char *vnlv[] = {vnl, (char *) NULL};
	return ioth_newstackv(stack, vnlv);
}

struct ioth *ioth_newstackl(const char *stack, const char *vnl, ... /* (char  *) NULL */) {
	if (vnl == (char *) NULL)
		return ioth_newstack(stack);
	else {
		va_list ap;
		int countargs;
		va_start(ap, vnl);
		for (countargs = 1; va_arg(ap, const char *) != (char *) NULL; countargs++)
			;
		va_end(ap);
		const char *vnlv[countargs + 1];
		vnlv[0] = vnl;
		va_start(ap, vnl);
		for (countargs = 1;
				(vnlv[countargs] = va_arg(ap, const char *)) != (char *) NULL;
				countargs++)
			;
		va_end(ap);
		return ioth_newstackv(stack, vnlv);
	}
}

static inline struct ioth *ioth_getstack(int fd) {
	struct ioth **ioth = fduserdata_get(fdtable, fd);
	if (ioth == NULL)
		return NULL;
	struct ioth *iothstack = *ioth;
	fduserdata_put(ioth);
	stackdata = iothstack->stackdata;
	return iothstack;
}

int ioth_msocket(struct ioth *iothstack, int domain, int type, int protocol) {
	int fd;
	if (iothstack == NULL)
		iothstack = &default_iothstack;
	iothstack->count++;
	stackdata = iothstack->stackdata;
	if (iothstack->f.socket == NULL)
		return errno = ENOSYS, -1;
	fd = iothstack->f.socket(domain, type, protocol);
	if (fd < 0)
		iothstack->count--;
	else {
		struct ioth **ioth = fduserdata_new(fdtable, fd, struct ioth *);
		*ioth = iothstack;
		fduserdata_put(ioth);
	}
	return fd;
}

#define IOTH_getiothstack_ck(fd, fun) \
	struct ioth *iothstack = ioth_getstack(fd); \
	if (iothstack == NULL) \
	return errno = EBADF, -1; \
	if (iothstack->f.fun == NULL) \
	return errno = ENOSYS, -1

int ioth_close(int fd) {
	int retval;
	struct ioth **ioth = fduserdata_get(fdtable, fd);
	if (ioth == NULL)
		return errno = ENOSYS, -1;
	struct ioth *iothstack = *ioth;
	if (iothstack->f.close == NULL)
		return errno = ENOSYS, -1;
	retval = iothstack->f.close(fd);
	if (retval == 0) {
		iothstack->count--;
		fduserdata_del(ioth);
	} else
		fduserdata_put(ioth);
	return retval;
}

int ioth_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	int newfd;
	IOTH_getiothstack_ck(fd, accept);
	newfd = iothstack->f.accept(fd, addr, addrlen);
	if (newfd >= 0) {
		struct ioth **ioth = fduserdata_new(fdtable, newfd, struct ioth *);
		*ioth = iothstack;
		iothstack->count++;
		fduserdata_put(ioth);
	}
	return newfd;
}

static ssize_t _ioth_read(struct ioth *iothstack, int fd, void *buf, size_t len);
ssize_t _ioth_readv(struct ioth *iothstack, int fd, const struct iovec *iov, int iovcnt);
ssize_t _ioth_recv(struct ioth *iothstack, int fd, void *buf, size_t len, int flags);
ssize_t _ioth_recvfrom(struct ioth *iothstack, int fd, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen);
ssize_t _ioth_recvmsg(struct ioth *iothstack, int fd, struct msghdr *msg, int flags);
ssize_t _ioth_write(struct ioth *iothstack, int fd, const void *buf, size_t size);
ssize_t _ioth_writev(struct ioth *iothstack, int fd, const struct iovec *iov, int iovcnt);
ssize_t _ioth_send(struct ioth *iothstack, int fd, const void *buf, size_t size, int flags);
ssize_t _ioth_sendto(struct ioth *iothstack, int fd, const void *buf, size_t size, int flags,
		const struct sockaddr *to, socklen_t tolen);
ssize_t _ioth_sendmsg(struct ioth *iothstack, int fd, const struct msghdr *msg, int flags);

static ssize_t _ioth_read(struct ioth *iothstack, int fd, void *buf, size_t len) {
	if (iothstack->f.read)
		return iothstack->f.read(fd, buf, len);
	else
		return _ioth_recv(iothstack, fd, buf, len, 0);
}

ssize_t _ioth_readv(struct ioth *iothstack, int fd, const struct iovec *iov, int iovcnt) {
	if (iothstack->f.readv)
		return iothstack->f.readv(fd, iov, iovcnt);
	else if (iothstack->f.recvmsg) {
		struct msghdr mhdr = { .msg_iov = (struct iovec *)iov, .msg_iovlen = iovcnt };
		return iothstack->f.recvmsg(fd, &mhdr, 0);
	} else // map to read
		return errno = ENOSYS, -1;
}

ssize_t _ioth_recv(struct ioth *iothstack, int fd, void *buf, size_t len, int flags) {
	if (iothstack->f.recv)
		return iothstack->f.recv(fd, buf, len, flags);
	else
		return _ioth_recvfrom(iothstack, fd, buf, len, flags, NULL, NULL);
}

ssize_t _ioth_recvfrom(struct ioth *iothstack, int fd, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen) {
	if (iothstack->f.recvfrom)
		return iothstack->f.recvfrom( fd, buf, len, flags, from, fromlen);
	else if (iothstack->f.recvmsg) {
		struct iovec iov[] = {{buf, len}};
		struct msghdr mhdr = {
			.msg_name = from,
			.msg_namelen = (fromlen) ? *fromlen : 0,
			.msg_iov = iov,
			.msg_iovlen = 1};
		ssize_t retval = iothstack->f.recvmsg(fd, &mhdr, flags);
		if (retval >= 0 && fromlen) *fromlen = mhdr.msg_namelen;
		return retval;
	} else
		return errno = ENOSYS, -1;
}

ssize_t _ioth_recvmsg(struct ioth *iothstack, int fd, struct msghdr *msg, int flags) {
	if (iothstack->f.recvmsg) {
		return iothstack->f.recvmsg(fd, msg, flags);
	} else
		return errno = ENOSYS, -1;
}

ssize_t _ioth_write(struct ioth *iothstack, int fd, const void *buf, size_t len) {
	if (iothstack->f.write)
		return iothstack->f.write(fd, buf, len);
	else
		return _ioth_send(iothstack, fd, buf, len, 0);
}

ssize_t _ioth_writev(struct ioth *iothstack, int fd, const struct iovec *iov, int iovcnt) {
	if (iothstack->f.writev)
		return iothstack->f.writev(fd, iov, iovcnt);
	else if (iothstack->f.sendmsg) {
		struct msghdr mhdr = { .msg_iov = (struct iovec *)iov, .msg_iovlen = iovcnt };
		return iothstack->f.sendmsg(fd, &mhdr, 0);
	} else // map to write
		return errno = ENOSYS, -1;
}

ssize_t _ioth_send(struct ioth *iothstack, int fd, const void *buf, size_t len, int flags) {
	if (iothstack->f.send)
		return iothstack->f.send(fd, buf, len, flags);
	else
		return _ioth_sendto(iothstack, fd, buf, len, flags, NULL, 0);
}

ssize_t _ioth_sendto(struct ioth *iothstack, int fd, const void *buf, size_t len, int flags,
		const struct sockaddr *to, socklen_t tolen) {
	if (iothstack->f.sendto)
		return iothstack->f.sendto( fd, buf, len, flags, to, tolen);
	else if (iothstack->f.sendmsg) {
		struct iovec iov[] = {{(void *)buf, (size_t)len}};
		struct msghdr mhdr = {
			.msg_name = (struct sockaddr *) to,
			.msg_namelen = tolen,
			.msg_iov = iov,
			.msg_iovlen = 1};
		return iothstack->f.sendmsg(fd, &mhdr, flags);
	} else
		return errno = ENOSYS, -1;
}

ssize_t _ioth_sendmsg(struct ioth *iothstack, int fd, const struct msghdr *msg, int flags) {
	if (iothstack->f.sendmsg) {
		return iothstack->f.sendmsg(fd, msg, flags);
	} else
		return errno = ENOSYS, -1;
}

#define IOTH_stackfun(fd, fun) \
	struct ioth *iothstack = ioth_getstack(fd); \
	if (iothstack == NULL) \
	return errno = EBADF, -1; \
	return _ioth_ ## fun

ssize_t ioth_read(int fd, void *buf, size_t len) {
	IOTH_stackfun(fd, read) (iothstack, fd, buf, len);
}

ssize_t ioth_readv(int fd, const struct iovec *iov, int iovcnt) {
	IOTH_stackfun(fd, readv) (iothstack, fd, iov, iovcnt);
}

ssize_t ioth_recv(int fd, void *buf, size_t len, int flags) {
	IOTH_stackfun(fd, recv) (iothstack, fd, buf, len, flags);
}

ssize_t ioth_recvfrom(int fd, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen) {
	IOTH_stackfun(fd, recvfrom) (iothstack, fd, buf, len, flags, from, fromlen);
}

ssize_t ioth_recvmsg(int fd, struct msghdr *msg, int flags) {
	IOTH_stackfun(fd, recvmsg) (iothstack, fd, msg, flags);
}

ssize_t ioth_write(int fd, const void *buf, size_t len) {
	IOTH_stackfun(fd, write) (iothstack, fd, buf, len);
}

ssize_t ioth_writev(int fd, const struct iovec *iov, int iovcnt) {
	IOTH_stackfun(fd, writev) (iothstack, fd, iov, iovcnt);
}

ssize_t ioth_send(int fd, const void *buf, size_t len, int flags) {
	IOTH_stackfun(fd, send) (iothstack, fd, buf, len, flags);
}

ssize_t ioth_sendto(int fd, const void *buf, size_t len, int flags,
		const struct sockaddr *to, socklen_t tolen) {
	IOTH_stackfun(fd, sendto) (iothstack, fd, buf, len, flags, to, tolen);
}

ssize_t ioth_sendmsg(int fd, const struct msghdr *msg, int flags) {
	IOTH_stackfun(fd, sendmsg) (iothstack, fd, msg, flags);
}

#define IOTH_fwfun(fd, fun) \
	IOTH_getiothstack_ck(fd, fun); \
	return iothstack->f.fun

int ioth_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
	IOTH_fwfun(fd, bind) (fd, addr, addrlen);
}

int ioth_connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
	IOTH_fwfun(fd, connect) (fd, addr, addrlen);
}

int ioth_listen(int fd, int backlog) {
	IOTH_fwfun(fd, listen) (fd, backlog);
}

int ioth_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	IOTH_fwfun(fd, getsockname) (fd, addr, addrlen);
}

int ioth_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	IOTH_fwfun(fd, getpeername) (fd, addr, addrlen);
}

int ioth_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
	IOTH_fwfun(fd, setsockopt) (fd, level, optname, optval, optlen);
}

int ioth_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen) {
	IOTH_fwfun(fd, getsockopt) (fd, level, optname, optval, optlen);
}

int ioth_shutdown(int fd, int how) {
	IOTH_fwfun(fd, shutdown) (fd, how);
}

int ioth_ioctl(int fd, unsigned long cmd, void *argp) {
	IOTH_fwfun(fd, ioctl) (fd, cmd, argp);
}

int ioth_fcntl(int fd, int cmd, long val) {
	IOTH_fwfun(fd, fcntl) (fd, cmd, val);
}

__attribute__((constructor))
	static void init(void) {
		fdtable = fduserdata_create(0);
	}

__attribute__((destructor))
	static void fini(void) {
		fduserdata_destroy(fdtable);
	}

/*
	 int main() {
//ioth_newstacki("picox", "vxvde://");
//ioth_newstackl("picox", "vde://", NULL);
//ioth_newstackl("picox", "vde://", "vxvde://234.0.0.0.1", "tap://tap0", NULL);
return 0;
}*/

