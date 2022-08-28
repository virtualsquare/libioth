/*
 *   libioth: choose your networking library as a plugin at run time.
 *
 *   Copyright (C) 2022  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
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
#include <string.h>

#define SPDX_PREFIX "SPDX-License-Identifier: "
#define SPDX_PREFIXLEN (sizeof(SPDX_PREFIX) - 1)

/* This implementation manages only a small subset of possible license
 * combinations. */

int checklicense(const char *prglicense, const char *liblicense) {
	/* No lib license restrictions: ok to load */
	if (liblicense == NULL) return 1;
	if (strncmp(liblicense, SPDX_PREFIX, SPDX_PREFIXLEN) == 0)
			liblicense += SPDX_PREFIXLEN;
	/* It is a permissive library */
	if (strncasecmp(liblicense, "LGPL-", 5) == 0)
		return 1;
	/* Lib has requirements, no license spec from prg -> deny */
	if (prglicense == NULL) return 0;
	if (strncmp(prglicense, SPDX_PREFIX, SPDX_PREFIXLEN) == 0)
			prglicense += SPDX_PREFIXLEN;
	/* It is the very same license: OK */
	if (strcasecmp(prglicense, liblicense) == 0) return 1;
	if (strcasecmp(liblicense, "GPL-2.0-or-later") == 0) {
		if (strcasecmp(prglicense, "GPL-2.0-only") == 0 ||
				strcasecmp(prglicense, "GPL-3.0-only") == 0 ||
				strcasecmp(prglicense, "GPL-2.0-or-later") == 0 ||
				strcasecmp(prglicense, "GPL-3.0-or-later") == 0)
			return 1;
		return 0;
	}
	if (strcasecmp(liblicense, "GPL-3.0-or-later") == 0) {
		if (strcasecmp(prglicense, "GPL-3.0-only") == 0 ||
				strcasecmp(prglicense, "GPL-3.0-or-later") == 0)
			return 1;
		return 0;
	}
	if (strcasecmp(liblicense, "GPL-2.0-only OR GPL-3.0-only") == 0) {
		if (strcasecmp(prglicense, "GPL-2.0-only") == 0 ||
				strcasecmp(prglicense, "GPL-3.0-only") == 0 ||
				strcasecmp(prglicense, "GPL-2.0-or-later") == 0 ||
				strcasecmp(prglicense, "GPL-3.0-or-later") == 0)
			return 1;
		return 0;
	}
	/* in doubt deny */
	return 0;
}

#if 0
int main(int argc, char *argv[]) {
	printf("%d\n", licensecheck(argv[1], argv[2]));
}
#endif
