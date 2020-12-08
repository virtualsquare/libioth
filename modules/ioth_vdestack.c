/*
 *   libioth: choose your networking library as a plugin at run time.
 *   plugin for vdestack (vdestack borrows the kernel stack using a namespace)
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
#include <stdio.h>
#include <vunet.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/if_tun.h>
#include <libvdeplug.h>

#define APPSIDE 0
#define DAEMONSIDE 1

#define DEFAULT_IF_NAME "vde0"
#define POLLING_TIMEOUT 10000

#define CHILD_STACK_SIZE (256 * 1024)

struct vdestack {
	pid_t pid;
	pid_t parentpid;
	int cmdpipe[2]; // socketpair for commands;
	VDECONN *vdeconn;
	char *child_stack;
	char ifname[];
};

struct vdecmd {
	int domain;
	int type;
	int protocol;
};

struct vdereply {
	int rval;
	int err;
};

static int open_tap(char *name) {
	struct ifreq ifr;
	int fd=-1;
	if((fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC)) < 0)
		return -1;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
	if(ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static int childFunc(void *arg)
{
	struct vdestack *stack = arg;
	int n;
	char buf[VDE_ETHBUFSIZE];
	int tapfd = open_tap(stack->ifname);
	VDECONN *conn = stack->vdeconn;
	struct pollfd pfd[] = {{stack->cmdpipe[DAEMONSIDE], POLLIN, 0},
		{tapfd, POLLIN, 0},
		{vde_datafd(conn), POLLIN, 0}};
	if (tapfd  < 0) {
		perror("tapfd"); _exit(1);
	}
	pfd[1].fd = tapfd;
	while (poll(pfd, 3, POLLING_TIMEOUT) >= 0) {
		// printf("poll in %d %d %d\n",pfd[0].revents,pfd[1].revents,pfd[2].revents);
		if (kill(stack->parentpid, 0) < 0)
			break;
		if (pfd[0].revents & POLLIN) {
			struct vdecmd cmd;
			struct vdereply reply;
			int n;
			if ((n = read(stack->cmdpipe[DAEMONSIDE], &cmd, sizeof(cmd))) > 0) {
				reply.rval = socket(cmd.domain, cmd.type, cmd.protocol);
				reply.err = errno;
				write(stack->cmdpipe[DAEMONSIDE], &reply, sizeof(reply));
			} else
				break;
		}
		if (pfd[1].revents & POLLIN) {
			n = read(tapfd, buf, VDE_ETHBUFSIZE);
			if (n == 0) break;
			vde_send(conn, buf, n, 0);
		}
		if (pfd[2].revents & POLLIN) {
			n = vde_recv(conn, buf, VDE_ETHBUFSIZE, 0);
			if (n == 0) break;
			write(tapfd, buf, n);
		}
		//printf("poll out\n");
	}
	close(stack->cmdpipe[DAEMONSIDE]);
	_exit(EXIT_SUCCESS);
}

struct vdestack *vde_addstack(char *vdenet, char *ifname) {
	char *ifnameok = ifname ? ifname : DEFAULT_IF_NAME;
	size_t ifnameoklen = strlen(ifnameok);
	struct vdestack *stack = malloc(sizeof(*stack) + ifnameoklen + 1);

	if (stack) {
		strncpy(stack->ifname, ifnameok, ifnameoklen + 1);
		stack->child_stack =
			mmap(0, CHILD_STACK_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (stack->child_stack == NULL)
			goto err_child_stack;

		if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, stack->cmdpipe) < 0)
			goto err_cmdpipe;

		if ((stack->vdeconn = vde_open(vdenet, "vdestack", NULL)) == NULL)
			goto err_vdenet;

		stack->parentpid = getpid();
		stack->pid = clone(childFunc, stack->child_stack + CHILD_STACK_SIZE,
				CLONE_FILES | CLONE_NEWUSER | CLONE_NEWNET | SIGCHLD, stack);
		if (stack->pid == -1)
			goto err_child;
	}
	return stack;
err_child:
err_vdenet:
	close(stack->cmdpipe[APPSIDE]);
	close(stack->cmdpipe[DAEMONSIDE]);
err_cmdpipe:
	munmap(stack->child_stack, CHILD_STACK_SIZE);
err_child_stack:
	free(stack);
	return NULL;
}

void vde_delstack(struct vdestack *stack) {
	vde_close(stack->vdeconn);
	close(stack->cmdpipe[APPSIDE]);
	waitpid(stack->pid, NULL, 0);
	munmap(stack->child_stack, CHILD_STACK_SIZE);
	free(stack);
}

int vde_msocket(struct vdestack *stack, int domain, int type, int protocol) {
	struct vdecmd cmd = {domain, type, protocol};
	struct vdereply reply;

	write(stack->cmdpipe[APPSIDE],  &cmd, sizeof(cmd));
	read(stack->cmdpipe[APPSIDE], &reply, sizeof(reply));

	if (reply.rval < 0)
		errno = reply.err;
	return reply.rval;
}

static typeof(getstackdata_prototype) *getstackdata;

void *ioth_vdestack_newstack(const char *vnlv[], const char *options,
		struct ioth_functions *ioth_f) {
	const char *vnl = (vnlv) ? vnlv[0] : NULL;
	struct vdestack *stackdata = vde_addstack((char *) vnl, NULL);
	getstackdata = ioth_f->getstackdata;
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
	return stackdata;
}

int ioth_vdestack_delstack(void *stackdata) {
	vde_delstack((struct vdestack *) stackdata);
	return 0;
}

int ioth_vdestack_socket(int domain, int type, int protocol) {
	struct vdestack *stackdata = getstackdata();
	return vde_msocket(stackdata, domain, type, protocol);
}

void *ioth_vdestack_n_newstack(const char *vnlv[], const char *options,
		struct ioth_functions *ioth_f)
  __attribute__ ((alias ("ioth_vdestack_newstack")));
int ioth_vdestack_n_delstack(void *stackdata)
  __attribute__ ((alias ("ioth_vdestack_delstack")));
int ioth_vdestack_n_socket(int domain, int type, int protocol)
  __attribute__ ((alias ("ioth_vdestack_socket")));
