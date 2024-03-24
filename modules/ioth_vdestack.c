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

#include <ioth.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
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
#define ETH_HEADER_SIZE 14

#define CHILD_STACK_SIZE (256 * 1024)

const char *ioth_vdestack_license = "SPDX-License-Identifier: LGPL-2.1-or-later";

struct vdestack {
	pid_t pid;
	pid_t parentpid;
	int noif;
	pthread_mutex_t mutex;
	int cmdpipe[2]; // socketpair for commands;
	char *child_stack;
	struct {
		VDECONN *vdeconn;
		char ifname[IFNAMSIZ];
	} iface[];
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
		perror(name);
		close(fd);
		return -1;
	}
	return fd;
}

static int childFunc(void *arg)
{
	struct vdestack *stack = arg;
	int noif = stack->noif;
	struct pollfd pfd[noif * 2 + 1];
	int i;
	ssize_t unused;
	for (i = 0; i < noif; i++) {
    pfd[i].fd = vde_datafd(stack->iface[i].vdeconn);
    pfd[i + noif].fd = open_tap(stack->iface[i].ifname);
    pfd[i].events = pfd[i + noif].events = POLLIN;
    pfd[i].revents = pfd[i + noif].revents = 0;
  }
  pfd[noif * 2].fd = stack->cmdpipe[DAEMONSIDE];
  pfd[noif * 2].events = POLLIN;
  pfd[noif * 2].revents = 0;
	while (poll(pfd, noif * 2 + 1, POLLING_TIMEOUT) >= 0) {
		char buf[VDE_ETHBUFSIZE];
		size_t n;
		// printf("poll in %d %d %d\n",pfd[0].revents,pfd[1].revents,pfd[2].revents);
		if (kill(stack->parentpid, 0) < 0)
			break;
		if (pfd[noif * 2].revents & POLLIN) {
			struct vdecmd cmd;
			struct vdereply reply;
			if ((n = read(stack->cmdpipe[DAEMONSIDE], &cmd, sizeof(cmd))) > 0) {
				reply.rval = socket(cmd.domain, cmd.type, cmd.protocol);
				reply.err = errno;
				unused = write(stack->cmdpipe[DAEMONSIDE], &reply, sizeof(reply));
			} else
				break;
		}
		for (i = 0; i < noif; i++) {
			if (pfd[i + noif].revents & POLLIN) {
				n = read(pfd[i + noif].fd, buf, VDE_ETHBUFSIZE);
				if (n <= 0) break;
				vde_send(stack->iface[i].vdeconn, buf, n, 0);
			}
			if (pfd[i].revents & POLLIN) {
				n = vde_recv(stack->iface[i].vdeconn, buf, VDE_ETHBUFSIZE, 0);
				if (n <= 0) break;
				if (n >= ETH_HEADER_SIZE)
					unused = write(pfd[i + noif].fd, buf, n);
			}
		}
		(void) unused;
		//printf("poll out\n");
	}
	for (i = 0; i < noif; i++) {
		if (pfd[i + noif].fd >= 0)
			close(pfd[i + noif].fd);
	}
	close(stack->cmdpipe[DAEMONSIDE]);
	_exit(EXIT_SUCCESS);
}

static int countif(const char **v) {
	int count;
	if (v == NULL) return 0;
	for (count = 0; v[count]; count++)
		;
	return count;
}

struct vdestack *vde_addstack(const char *vnlv[], const char *options) {
	(void) options;
	int i;
	int noif = countif(vnlv);
	struct vdestack *stack = malloc(sizeof(*stack) + sizeof(stack->iface[0]) * noif);
	if (stack) {
		//printf("noif %d\n",noif);
		stack->noif = noif;
		if (pthread_mutex_init(&stack->mutex, NULL) != 0)
			goto err_mutex;
		stack->child_stack =
			mmap(0, CHILD_STACK_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (stack->child_stack == NULL)
			goto err_child_stack;

		if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, stack->cmdpipe) < 0)
			goto err_cmdpipe;

		for (i = 0; i < noif; i++)
			stack->iface[i].vdeconn = NULL;

		for (i = 0; i < noif; i++) {
			const char *ifvnl = vnlv[i];
			char *delim = strstr(ifvnl, "://");  // position of "://"
			char *colonmark = strchr(ifvnl, ':'); // position of ':'
			if (colonmark && (!delim  || (delim && colonmark < delim))) {
				/* spit ifname from vnl */
				int ifnamelen = colonmark - ifvnl;
				snprintf(stack->iface[i].ifname, IFNAMSIZ, "%.*s", ifnamelen, ifvnl);
				ifvnl = colonmark + 1;
			} else
				snprintf(stack->iface[i].ifname, IFNAMSIZ, "vde%d", i);
			//printf("open %s %s\n", stack->iface[i].ifname,  ifvnl);
			if ((stack->iface[i].vdeconn = vde_open((char *) ifvnl, "ioth_vdestack", NULL)) == NULL)
				goto err_vdenet;
		}

		stack->parentpid = getpid();
		stack->pid = clone(childFunc, stack->child_stack + CHILD_STACK_SIZE,
				CLONE_FILES | CLONE_NEWUSER | CLONE_NEWNET | SIGCHLD, stack);
		if (stack->pid == -1)
			goto err_child;
	}
	return stack;
err_child:
err_vdenet:
	for (i = 0; i < noif; i++) {
		if (stack->iface[i].vdeconn)
			vde_close(stack->iface[i].vdeconn);
	}
	close(stack->cmdpipe[APPSIDE]);
	close(stack->cmdpipe[DAEMONSIDE]);
err_cmdpipe:
	munmap(stack->child_stack, CHILD_STACK_SIZE);
err_child_stack:
	pthread_mutex_destroy(&stack->mutex);
err_mutex:
	free(stack);
	return NULL;
}

void vde_delstack(struct vdestack *stack) {
	int i;
	int noif = stack->noif;
	for (i = 0; i < noif; i++) {
		if (stack->iface[i].vdeconn)
			vde_close(stack->iface[i].vdeconn);
	}
	close(stack->cmdpipe[APPSIDE]);
	waitpid(stack->pid, NULL, 0);
	munmap(stack->child_stack, CHILD_STACK_SIZE);
	pthread_mutex_destroy(&stack->mutex);
	free(stack);
}

int vde_msocket(struct vdestack *stack, int domain, int type, int protocol) {
	struct vdecmd cmd = {domain, type, protocol};
	struct vdereply reply;

	pthread_mutex_lock(&stack->mutex);
	if (write(stack->cmdpipe[APPSIDE],  &cmd, sizeof(cmd)) < 0 ||
			read(stack->cmdpipe[APPSIDE], &reply, sizeof(reply)) < 0)
		goto err;
	pthread_mutex_unlock(&stack->mutex);

	if (reply.rval < 0)
		errno = reply.err;
	return reply.rval;
err:
	pthread_mutex_unlock(&stack->mutex);
	return -1;
}

static typeof(getstackdata_prototype) *getstackdata;

void *ioth_vdestack_newstack(const char *vnlv[], const char *options,
		struct ioth_functions *ioth_f) {
	struct vdestack *stackdata = vde_addstack(vnlv, options);
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
