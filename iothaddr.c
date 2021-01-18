/*
 *   iothaddr.c: utility library for ioth address management
 *       hash (md5sum) based mac and ipv6 host address + eui64 conversion
 *
 *   Copyright 2021 Renzo Davoli - Virtual Square Team
 *   University of Bologna - Italy
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

#include <mhash.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <iothaddr.h>

void iothaddr_hash(void *addr, const char *name, const char *passwd, uint32_t otiptime) {
	struct in6_addr *addr6 = addr;
	size_t namelen = strlen(name);
	MHASH td;
	char out[mhash_get_block_size(MHASH_MD5)];
	int i;
	memset(out, 0, mhash_get_block_size(MHASH_MD5));
	if (name[namelen-1] == '.') namelen--;
	td=mhash_init(MHASH_MD5);
	mhash(td, name, namelen);
	if (passwd != NULL)
		mhash(td, passwd, strlen(passwd));
	if (otiptime != 0) {
		uint32_t otiptime_n = htonl(otiptime);
		mhash(td, &otiptime_n, sizeof(otiptime_n));
	}
	mhash_deinit(td, out);
	for (i=8; i<16; i++)
		addr6->s6_addr[i] ^= out[i-8];
	addr6->s6_addr[8] &= ~0x3;   // locally adm, unicast
}

void iothaddr_hashmac(void *mac, const char *name, const char *passwd) {
	unsigned char *umac = mac;
	size_t namelen = strlen(name);
	MHASH td;
	char out[mhash_get_block_size(MHASH_MD5)];
	int i;
	memset(out, 0, mhash_get_block_size(MHASH_MD5));
	if (name[namelen-1] == '.') namelen--;
	td=mhash_init(MHASH_MD5);
	mhash(td, name, namelen);
	if (passwd != NULL)
		mhash(td, passwd, strlen(passwd));
	mhash_deinit(td, out);
	for (i=0; i<3; i++)
		umac[i] = out[i];
	for (i=3; i<6; i++)
		umac[i] = out[i+2];
	umac[0] |= 0x2; // locally adm
	umac[0] &= ~0x1; // unicast
}

void iothaddr_eui64(void *addr, void *mac) {
	struct in6_addr *addr6 = addr;
	unsigned char *umac = mac;
	int i;
	for (i=0; i<3; i++)
		addr6->s6_addr[i + 8] = umac[i];
	addr6->s6_addr[11] = 0xff;
	addr6->s6_addr[12] = 0xfe;
	for (i=3; i<6; i++)
		addr6->s6_addr[i + 10] ^= umac[i];
	addr6->s6_addr[8] ^= 0x2;   // L bit has inverse meaning.
}

void iothaddr_hasheui64(void *addr, const char *name, const char *passwd) {
	unsigned char mac[6];
	iothaddr_hashmac(mac, name, passwd);
	iothaddr_eui64(addr, mac);
}
