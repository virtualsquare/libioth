<!--
.\" Copyright (C) 2022 VirtualSquare. Project Leader: Renzo Davoli
.\"
.\" This is free documentation; you can redistribute it and/or
.\" modify it under the terms of the GNU General Public License,
.\" as published by the Free Software Foundation, either version 2
.\" of the License, or (at your option) any later version.
.\"
.\" The GNU General Public License's references to "object code"
.\" and "executables" are to be interpreted as the output of any
.\" document formatting or typesetting system, including
.\" intermediate and printed output.
.\"
.\" This manual is distributed in the hope that it will be useful,
.\" but WITHOUT ANY WARRANTY; without even the implied warranty of
.\" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
.\" GNU General Public License for more details.
.\"
.\" You should have received a copy of the GNU General Public
.\" License along with this manual; if not, write to the Free
.\" Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
.\" MA 02110-1301 USA.
.\"
-->

# NAME

ioth_newstack, ioth_newstackl, ioth_newstackv, ioth_delstack, ioth_msocket,
ioth_set_defstack, ioth_get_defstack, ioth_socket,
ioth_close, ioth_bind, ioth_connect, ioth_listen, ioth_accept,
ioth_getsockname, ioth_getpeername, ioth_setsockopt, ioth_getsockopt,
ioth_shutdown, ioth_ioctl, ioth_fcntl,
ioth_read, ioth_readv, ioth_recv, ioth_recvfrom, ioth_recvmsg,
ioth_write, ioth_writev, ioth_send, ioth_sendto ioth_sendmsg,
ioth_if_nametoindex, ioth_linksetupdown, ioth_ipaddr_add,
ioth_ipaddr_del, ioth_iproute_add, ioth_iproute_del,
ioth_iplink_add, ioth_iplink_del, ioth_linksetaddr, ioth_linkgetaddr -
Internet of Threads (IoTh) library

# SYNOPSIS
`#include *ioth.h*`

`struct ioth *ioth_newstack(const char *`_stack_`, const char *`_vnl_`);`

`struct ioth *ioth_newstackl(const char *`_stack_`, const char *`_vnl_`, ... );`

`struct ioth *ioth_newstackv(const char *`_stack_`, const char *`_vnlv_`[]);`

`int ioth_delstack(struct ioth *`_iothstack_`);`

`int ioth_msocket(struct ioth *`_iothstack_`, int ` _domain_`, int ` _type_`, int ` _protocol_`);`

`void ioth_set_defstack(struct ioth *`_iothstack_`);`

`struct ioth *ioth_get_defstack(void);`

`int ioth_socket(int ` _domain_`, int ` _type_`, int ` _protocol_`);`

+ Berkeley Sockets API

`int ioth_close(int ` _fd_`);`

`int ioth_bind(int ` _sockfd_`, const struct sockaddr *`_addr_`, socklen_t ` _addrlen_`);`

`int ioth_connect(int ` _sockfd_`, const struct sockaddr *`_addr_`, socklen_t ` _addrlen_`);`

`int ioth_listen(int ` _sockfd_`, int ` _backlog_`);`

`int ioth_accept(int ` _sockfd_`, struct sockaddr *restrict ` _addr_`, socklen_t *restrict ` _addrlen_`);`

`int ioth_getsockname(int ` _sockfd_`, struct sockaddr *restrict ` _addr_`, socklen_t *restrict ` _addrlen_`);`

`int ioth_getpeername(int ` _sockfd_`, struct sockaddr *restrict ` _addr_`, socklen_t *restrict ` _addrlen_`);`

`int ioth_getsockopt(int ` _sockfd_`, int ` _level_`, int ` _optname_`, void *restrict ` _optval_`, socklen_t *restrict ` _optlen_`);`

`int ioth_setsockopt(int ` _sockfd_`, int ` _level_`, int ` _optname_`, const void *`_optval_`, socklen_t ` _optlen_`);`

`int ioth_shutdown(int ` _sockfd_`, int ` _how_`);`

`int ioth_ioctl(int ` _fd_`, unsigned long ` _request_`, ...);`

`int ioth_fcntl(int ` _fd_`, int ` _cmd_`, ... /* arg */ );`

`ssize_t ioth_read(int ` _fd_`, void *`_buf_`, size_t ` _count_`);`

`ssize_t ioth_readv(int ` _fd_`, const struct iovec *`_iov_`, int ` _iovcnt_`);`

`ssize_t ioth_recv(int ` _sockfd_`, void *`_buf_`, size_t ` _len_`, int ` _flags_`);`

`ssize_t ioth_recvfrom(int ` _sockfd_`, void *restrict ` _buf_`, size_t ` _len_`, int ` _flags_`, struct sockaddr *restrict ` _src_addr_`, socklen_t *restrict ` _addrlen_`);`

`ssize_t ioth_recvmsg(int ` _sockfd_`, struct msghdr *`_msg_`, int ` _flags_`);`

`ssize_t ioth_write(int ` _fd_`, const void *`_buf_`, size_t ` _count_`);`

`ssize_t ioth_writev(int ` _fd_`, const struct iovec *`_iov_`, int ` _iovcnt_`);`

`ssize_t ioth_send(int ` _sockfd_`, const void *`_buf_`, size_t ` _len_`, int ` _flags_`);`

`ssize_t ioth_sendto(int ` _sockfd_`, const void *`_buf_`, size_t ` _len_`, int ` _flags_`, const struct sockaddr *`_dest_addr_`, socklen_t ` _addrlen_`);`

`ssize_t ioth_sendmsg(int ` _sockfd_`, const struct msghdr *`_msg_`, int ` _flags_`);`

+ nlinline+ API

`int ioth_if_nametoindex(const char *`_ifname_`);`

`int ioth_linksetupdown(unsigned int ` _ifindex_`, int ` _updown_`);`

`int ioth_ipaddr_add(int ` _family_`, void *`_addr_`, int ` _prefixlen_`, unsigned int ` _ifindex_`);`

`int ioth_ipaddr_del(int ` _family_`, void *`_addr_`, int ` _prefixlen_`, unsigned int ` _ifindex_`);`

`int ioth_iproute_add(int ` _family_`, void *`_dst_addr_`, int ` _dst_prefixlen_`, void *`_gw_addr_`, unsigned int ` _ifindex_`);`

`int ioth_iproute_del(int ` _family_`, void *`_dst_addr_`, int ` _dst_prefixlen_`, void *`_gw_addr_`, unsigned int ` _ifindex_`);`

`int ioth_iplink_add(const char *`_ifname_`, unsigned int ` _ifindex_`, const char *`_type_`, const char *`_data_`);`

`int ioth_iplink_del(const char *`_ifname_`, unsigned int ` _ifindex_`);

`int ioth_linksetaddr(unsigned int ` _ifindex_`, void *`_macaddr_`);

`int ioth_linkgetaddr(unsigned int ` _ifindex_`, void *`_macaddr_`);

# DESCRIPTION

`libioth` is the API for the Internet of Threads. TCP-IP networking stacks
can be loaded as dynamic libraries at run time.

* the API is minimal: Berkeley Sockets + msocket + newstack/delstack.
* the stack implementation can be chosen as a plugin at run time.
* netlink based stack/interface/ip configuration via nlinline.
* ioth sockets are real file descriptors, poll/select/ppoll/pselect/epoll friendly
* plug-ins can be loaded in private address namespaces: libioth supports several stacks of the same type (same plugin) even if the stack implementation library was designed to provide just one stack.

`libioth` provides the following functions:

  `ioth_newstack`
: `ioth_newstack` creates a new stack without any interface if `vnl` is NULL, otherwise the new stack has a virtual interface connected to the vde network identified by the VNL (Virtual Network Locator, see vde_open(3) ).

  `ioth_newstackl`, `ioth_newstackv`
: ioth_newstackl` and `ioth_newstackv` (l = list, v = vector) support the creation of a new stack with several interfaces. It is possible to provide the VNLs as a sequence of arguments (as in execl(3)) or as a NULL terminated array of VNLs (as the arguments in execv(3)).

  `ioth_delstack`
: This function terminates/deletes a stack.

  `ioth_msocket`
: This is the multi-stack supporting extension of socket(2). It behaves exactly as socket except for the added heading argument that allows the choice of the stack among those currently available (previously created by a `ioth_newstack*`).

  `ioth_set_defstack`, `ioth_get_defstack`
: These functions define and retrieve the default stack, respectively.
: The default stack is implicitly used by ioth_msocket when its first argument iothstack is NULL.
: The default stack is initially defined as the native stack provided by the kernel. Use ioth_set_defstack(mystack) to define mystack as the current default stack. ioth_set_defstack(NULL) to revert the default stack to the native stack.

  `ioth_socket`
: `ioth_socket` opens a socket using the default stack: `ioth_socket(d, t, p)` is an alias for `ioth_msocket(NULL, d, t, p)`

  `ioth_close`, `ioth_bind`, `ioth_connect`, `ioth_listen`, `ioth_accept`, `ioth_getsockname`, `ioth_getpeername`, `ioth_setsockopt`, `ioth_getsockopt`, `ioth_shutdown`, `ioth_ioctl`, `ioth_fcntl`, `ioth_read`, `ioth_readv`, `ioth_recv`, `ioth_recvfrom`, `ioth_recvmsg`, `ioth_write`, `ioth_writev`, `ioth_send`, `ioth_sendto`, `ioth_sendmsg`
: these functions have the same signature and functionalities of their counterpart in (2) and (3) without the `ioth_` prefix.

  `ioth_if_nametoindex`, `ioth_linksetupdown`, `ioth_ipaddr_add`, ` ioth_ipaddr_del`, `ioth_iproute_add`, `ioth_iproute_del`, ` ioth_iplink_add`, `ioth_iplink_del`, `ioth_linksetaddr`, `ioth_linkgetaddr`
: these functions have the same signature and functionnalities described in `nlinline`(3).

# RETURN VALUE

`ioth_newstack`, `ioth_newstackl`, `ioth_newstackv` return a `struct stack` pointer, NULL in case of
error. This address is used as a descriptor of the newly created stack
and is later passed as parameter to `ioth_msocket`, `ioth_set_defstack` or `ioth_delstack`.

`ioth_msocket` and `ioth_socket` return the file descriptor of the new socket, -1 in case of errore.

`ioth_delstack` returns -1 in case of error, 0 otherwise. If there are file descriptors already in use, this function fails and errno is EBUSY.

`ioth_get_defstack` returns the stack descriptor of the default stack.

The return values of all the other functions are defined in the man pages of the
corresponding functions provided by the GNU C library or nlinline(3)

# SEE ALSO

vde_plug(1), vdeplug_open(3), nlinline(3)

# AUTHOR

VirtualSquare. Project leader: Renzo Davoli
