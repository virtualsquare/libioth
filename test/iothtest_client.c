/*
 *   libioth: choose your networking library as a plugin at run time.
 *   test program: client
 *
 *   Copyright (C) 2020-2022  Renzo Davoli <renzo@cs.unibo.it>
 *                            VirtualSquare team.
 *
 * this test program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define SPDX_LICENSE "SPDX-License-Identifier: GPL-2.0-or-later"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ioth.h>

void *handle (void *arg) {
	int connfd = (uintptr_t) arg;
	int n;
	char buf[BUFSIZ];
	printf("new conn %d tid %d\n", connfd, gettid());
	for(;;) {
		if ((n = ioth_recv(connfd, buf, BUFSIZ, 0)) <= 0)
			break;
		printf("tid %d GOT: %*.*s",gettid(),n,n,buf);
		ioth_send(connfd, buf, n, 0);
	}
	printf("close conn %d tid %d\n", connfd, gettid());
	ioth_close(connfd);
	return NULL;
}

void client(struct ioth *mystack) {
	struct sockaddr_in servaddr;
	int fd;
	size_t n;
	char buf[BUFSIZ];

	fd = ioth_msocket(mystack, AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5000);
	inet_pton(AF_INET, "192.168.250.50", &servaddr.sin_addr);

	if (ioth_connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0) {
		struct pollfd pfd[]={ {STDIN_FILENO, POLLIN, 0}, {fd, POLLIN, 0} };
		for (;;) {
			poll(pfd, 2, -1);
			if (pfd[0].revents) {
				n = read(STDIN_FILENO, buf, 1024);
				if (n==0) break;
				ioth_write(fd, buf,n);
			}
			if (pfd[1].revents) {
				n = ioth_read(fd, buf, 1024);
				if (n==0) break;
				n = write(STDOUT_FILENO, buf, n);
			}
		}
	}
	ioth_close(fd);
	sleep(1);
}

struct ioth *net_setup(const char **args) {
	struct ioth *mystack;
	uint8_t ipv4addr[] = {192,168,250,51};
	uint8_t ipv4gw[] = {192,168,250,1};
	int ifindex;

	mystack = ioth_newstackv(args[0], args+1);
	if (mystack != NULL) {
		ifindex = ioth_if_nametoindex(mystack, "vde0");
		if (ifindex < 0)
			perror("nametoindex");
		else {
			if (ioth_linksetupdown(mystack, ifindex, 1) < 0)
				perror("link up");
			if (ioth_ipaddr_add(mystack, AF_INET, ipv4addr, 24, ifindex) < 0)
				perror("addr ipv4");
			if (ioth_iproute_add(mystack, AF_INET, NULL, 0, ipv4gw, 0) < 0)
				perror("route ipv4");
		}
	}
	return mystack;
}

int main(int argc, const char *argv[]) {
	ioth_set_license(SPDX_LICENSE);
	if (argc < 2) {
		fprintf(stderr, "Usage:\n\n\t%s stack [ vnl vnl ]\n\n", basename(argv[0]));
		exit(1);
	} else {
		struct ioth *mystack = net_setup(argv+1);
		// printf("%p\n", mystack);
		if (mystack) {
			client(mystack);
			ioth_delstack(mystack);
		} else
			perror("net_setup");
	}
}
