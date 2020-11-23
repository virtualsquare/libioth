#ifndef LIBIOTH_H
#define LIBIOTH_H
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <nlinline+.h>

struct ioth;

int ioth_msocket(struct ioth *iothstack, int domain, int type, int protocol);
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

struct ioth *ioth_newstack(const char *stack);
struct ioth *ioth_newstacki(const char *stack, const char *vnl);
struct ioth *ioth_newstackl(const char *stack, const char *vnl, ... /* (char  *) NULL */);
struct ioth *ioth_newstackv(const char *stack, const char *vnlv[]);
int ioth_delstack(struct ioth *iothstack);

NLINLINE_LIBMULTI(ioth_)
/* ----------------------------------- for ioth plugins */

struct ioth_functions;
void *newstack_prototype(const char *vnlv[], struct ioth_functions *ioth_f);
int delstack_prototype(void *stackdata);
void *getstackdata_prototype(void);

struct ioth_functions {
	typeof(getstackdata_prototype) *getstackdata;
	typeof(newstack_prototype) *newstack;
	typeof(delstack_prototype) *delstack;
	typeof(socket) *socket;
	typeof(close) *close;
	typeof(bind) *bind;
	typeof(connect) *connect;
	typeof(listen) *listen;
	typeof(accept) *accept;
	typeof(getsockname) *getsockname;
	typeof(getpeername) *getpeername;
	typeof(setsockopt) *setsockopt;
	typeof(getsockopt) *getsockopt;
	typeof(shutdown) *shutdown;
	typeof(ioctl) *ioctl;
	typeof(fcntl) *fcntl;
	typeof(read) *read;
	typeof(readv) *readv;
	typeof(recv) *recv;
	typeof(recvfrom) *recvfrom;
	typeof(recvmsg) *recvmsg;
	typeof(write) *write;
	typeof(writev) *writev;
	typeof(send) *send;
	typeof(sendto) *sendto;
	typeof(sendmsg) *sendmsg;
};
#endif

