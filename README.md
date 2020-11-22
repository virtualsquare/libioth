# libioth

## The definitive API for the Internet of Threads

* the API is minimal: Berkeley Sockets + msocket + newstack/delstack.
* the stack implementation can be chosen as a plugin at run time.
* netlink based stack/interface/ip configuration via _nlinline_.
* ioth sockets are real file descriptors, poll/select/ppoll/pselect/epoll friendly
* plug-ins are loaded in private address namespaces: libioth supports several stacks
of the same type (same plugin) even if the stack implementation library was designed to
provide just one stack.

## Compile and Install

Pre-requisites: `fduserdata`, `nlinline`.

The `ioth_vdestack` plugin is built only if `libvdestack` is installed.

The `ioth_picox` plugin is built only when picoxnet (and picotcp) are installed.

Libioth uses cmake. The standard building/installing procedure is:

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

An uninstaller is provided for your convenience. In the build direcotry run:
```
sudo make uninstall
```

## The API

### newstack

There are four flavours of `ioth_newstack`:
```C
struct ioth *ioth_newstack(const char *stack);
struct ioth *ioth_newstacki(const char *stack, const char *vnl);
struct ioth *ioth_newstackl(const char *stack, const char *vnl, ... /* (char  *) NULL */);
struct ioth *ioth_newstackv(const char *stack, const char *vnlv[]);
```
* `ioth_newstack` creates a new stack. This function does not create any interface.
* `ioth_newstacki` (i = immediate) creates a new stack with a virtual interface connected to the vde network identified by the VNL (Virtual Netowrk Locator, see vdeplug4).
* `ioth_newstackl` and `ioth_newstackv` (l = list, v = vector) support the creation of a new stack with several interfaces. It is possible to provide the VNLs as a sequence of arguments (as in execl) or as a NULL terminated array of VNLs (as the arguments in execv).

The return value is the ioth stack descriptors, NULL in case of error (errno can provide a more detailed description of the error).

### delstack

```C
int ioth_delstack(struct ioth *iothstack);
```
This function terminates/deletes a stack. It returns -1 in case of error, 0 otherwise. If there are file descriptors already in use, this function fils and errno is EBUSY.

### msocket
```C
int ioth_msocket(struct ioth *iothstack, int domain, int type, int protocol);
```
This is the multi-stack supporting extension of `socket`(2). It behaves exactly as `socket` except for the new heading argument that allows the choice of the stack among those currently available.

### for everything else... Berkeley Sockets

`ioth_close`,
`ioth_bind`,
`ioth_connect`,
`ioth_listen`,
`ioth_accept`,
`ioth_getsockname`,
`ioth_getpeername`,
`ioth_setsockopt`,
`ioth_getsockopt`,
`ioth_shutdown`,
`ioth_ioctl`,
`ioth_fcntl`,
`ioth_read`,
`ioth_readv`,
`ioth_recv`,
`ioth_recvfrom`,
`ioth_recvmsg`,
`ioth_write`,
`ioth_writev`,
`ioth_send`,
`ioth_sendto` and
`ioth_sendmsg` have the same signature and functionalities of their counterpart
 without the `ioth_` prefix.

### extra features for free: nlinline netlink configuration functions

`netlink+` provides a set of inline functions for the stack interface/ip address and route 
configuration:
```C
ioth_if_nametoindex(const char *ifname);
ioth_linksetupdown(unsigned int ifindex, int updown);
ioth_ipaddr_add(int family, void *addr, int prefixlen, unsigned int ifindex);
ioth_ipaddr_del(int family, void *addr, int prefixlen, unsigned int ifindex);
ioth_iproute_add(int family, void *dst_addr, int dst_prefixlen, void *gw_addr);
ioth_iproute_del(int family, void *dst_addr, int dst_prefixlen, void *gw_addr);
ioth_iplink_add(const char *ifname, unsigned int ifindex, const char *type, 
    const char *data);
ioth_iplink_del(const char *ifname, unsigned int ifindex);
```

a detailed description can be found in `nlinline`(3).

## Example: an IPv4 TCP echo server

The complete source code of this example is provided in this git repository: `iothtest_server.c`.
The code creates a virtual stack, activates the interface named `vde0`, sets the interface's address to 192.168.250.50/24 and runs a TCP echo server on port 5000.

## Example: an IPv4 TCP terminal client

The complete source code of this example is provided in this git repository: `iothtest_client.c`.
The code creates a virtual stack, activates the interface named `vde0`, sets the interface's address to 192.168.250.51/24 and runs a TCP terminal client trying to get connected to 192.168.250.50 port 5000.

## testing

* start a vde switch:
```bash
     vde_plug null:// switch:///tmp/sw
```

* in a second terminal run the server using one of the following commands:
```bash
     ./iothtest_server vdestack vde:///tmp/sw
     ./iothtest_server picox vde:///tmp/sw
     vdens vde:// ./iothtest_server kernel
```

* in a third terminal run the client, again the stack implementation can be decided by choosing
one of the following commands

```bash
     ./iothtest_client vdestack vde:///tmp/sw
     ./iothtest_client picox vde:///tmp/sw
     vdens vde:// ./iothtest_client kernel
```

* now whatever is typed in the client is echoed back, the serveer produces a log of open/closed connections and echoed messages.

## The API for plugin development

* When libioth is required to create a new stack of type `T`, it loads the plugin named `ioth_T.so` in the `ioth` subdirectory of the directory where `libioth.so` is installed, or
in the hidden subdirectory named `.ioth` of the user's home directory. for example the plugin for vdestack is named `ioth_vdestack.so`.

* When the plugin has been loaded, ioth searches and runs the function `ioth_T_newstack` (so in the example above `ioth_vdestack_newstack`) passing two parameters: the array on VNLs to define the virtual interfaces, and a structure whose field are function pointers: `ioth_functions`.
This structure includes `getstackdata`, `newstack`, `delstack` and all the functions of the Berkeley Sockets API.

* The function `getstackdata` is provided by libioth and can be saved (all the other function can call `getstackdata()` to retrieve the pointer returned by `ioth_T_newstack`).

* All the other function pointers (except `getstackdata`) can be assigned to their implementation. Alternatively, if the plugin defines functions prefixed by `ioth_T_`, these are automatically recognized and used.

Example (from `ioth_vdestack.c`):
```C
int ioth_vdestack_socket(int domain, int type, int protocol) {
  struct vdestack *stackdata = getstackdata();
  return vde_msocket(stackdata, domain, type, protocol);
}
```
This function is automatically assigned to the field `socket` of the structure `ioth_functions`.
It retrieves the vdestack pointer using the getstackdata function as `vde_msocket` needs it as its
first argument.

* the plugin does not need any support library for libioth.