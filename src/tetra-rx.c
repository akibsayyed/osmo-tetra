/* Test program for tetra burst synchronizer */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/talloc.h>

#include "tetra_common.h"
#include <phy/tetra_burst.h>
#include <phy/tetra_burst_sync.h>
#include "tetra_gsmtap.h"

void *tetra_tall_ctx;

int main(int argc, char **argv)
{
	int fd;
	struct tetra_rx_state *trs;
	struct tetra_mac_state *tms;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file_with_1_byte_per_bit>\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}

	tetra_gsmtap_init("localhost", 0);

	tms = talloc_zero(tetra_tall_ctx, struct tetra_mac_state);
	tetra_mac_state_init(tms);

	trs = talloc_zero(tetra_tall_ctx, struct tetra_rx_state);
	trs->burst_cb_priv = tms;

#define	BUFSIZE 4096
	int to_consume = BUFSIZE;
	uint8_t buf[BUFSIZE];

	while (1) {
		int len;

		len = read(fd, buf, to_consume);
		if (len < 0) {
			perror("read");
			exit(1);
		}
		int rc = tetra_burst_sync_in(trs, buf, len);
		if (len == 0 && rc <= 0) {
			printf("EOF");
			break;
		}
		to_consume = MIN(abs(rc), BUFSIZE);
	}

	talloc_free(trs);
	talloc_free(tms);

	exit(0);
}
