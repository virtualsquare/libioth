#ifndef IOTHADDR_H
#define IOTHADDR_H
#include <stdint.h>
#include <time.h>

/* hash based IPv6 address:
	 the rightmost 64 bits of the address are XORed with the first 64 bit of
	 the md5sum of the concatenation of:
	 * name
	 * passwd, if passwd != NULL
	 * the big-endian 4 byte representation of otiptime, if otiptime  != 0.
	 bits 71 and 72 of addr are cleared (locally adm unicast address. */
/* e.g. addr: 2000:760::/64, name = "test.v2.cs.unibo.it", passwd = NULL, otiptime = 0
	 the md5sum of "test.v2.cs.unibo.it" is 69d62fac095a5a2ee4c2c79f211e57e3
	 The resulting address is 2000:760::68d6:2fac:95a:5a2e */
void iothaddr_hash(void *addr, const char *name, const char *passwd, uint32_t otiptime);

/* One Time IP address (OTIP), computation of otiptime:
	 ((seconds since the epoch) / otip_period) + otip_offset.
	 otip_period is the address expiration period.
	 otip_offset can be used on servers to anticipate the validity of addresses to tolerate
	 negative drifts or clients' clocks. */
static inline uint32_t iothaddr_otiptime(int otip_period, int otip_offset) {
  return (uint32_t) ((time(NULL) + otip_offset) / otip_period);
}

/* hash based mac address
	 Let H be the md5sum of the concatenation of:
   * name
   * passwd, if passwd != NULL.
	 The mac address is set to:
	 H'[0] : H[1] : H[2] : H[5] : H[6] : H[7]
	 (the first byte has the 7th bit set and the 8th cleared: locally adm unicast address)
	 e.g. name = "test.v2.cs.unibo.it", passwd = NULL
	 the md5sum of "test.v2.cs.unibo.it" is 69d62fac095a5a2ee4c2c79f211e57e3
	 the mac address is: 69:d6:2f:5a:5a:2e */
/* Hash based defined MAC address can avoid delays due to old info in arp tables
	 for process migration or restarting */
void iothaddr_hashmac(void *mac, const char *name, const char *passwd);

/* compute the EUI64 based IPv6 address from the mac address.
	 the rightmost 64 bits of the address are XORed with the EUI64 extension
	 of the 6 bytes mac address. */
void iothaddr_eui64(void *addr, void *mac);

/* compute the EUI64 based IPv6 address from the hash computed mac address.
	 This function computes iothaddr_eui64 on the result of iothaddr_hashmac */
/* e.g. addr: 2000:760::/64, name = "test.v2.cs.unibo.it", passwd = NULL
	 the md5sum of "test.v2.cs.unibo.it" is 69d62fac095a5a2ee4c2c79f211e57e3
   the mac address is: 69:d6:2f:5a:5a:2e
	 The resulting address is 2000:760::68d6:2fff:fe5a:5a2e */
void iothaddr_hasheui64(void *addr, const char *name, const char *passwd);

#endif
