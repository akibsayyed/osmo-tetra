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
#include <stdbool.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/talloc.h>

#include "tetra_common.h"
#include <phy/tetra_burst.h>
#include <phy/tetra_burst_sync.h>
#include "tetra_gsmtap.h"
#include "phy/tetra_burst_sync.h"

void *tetra_tall_ctx;

int main(int argc, char **argv)
{
	int fd;
	int opt;
	struct tetra_rx_state *trs;
	struct tetra_mac_state *tms;
	char *pcap_file_path = NULL;
	bool no_udp_tap = false;
	bool err = false;
	bool mcc_set = false;
	bool mnc_set = false;
	int hdist = 0;

	tms = talloc_zero(tetra_tall_ctx, struct tetra_mac_state);
	tetra_mac_state_init(tms);

	trs = talloc_zero(tetra_tall_ctx, struct tetra_rx_state);
	trs->burst_cb_priv = tms;

	while ((opt = getopt(argc, argv, "a:t:d:nuic:m:s:h:")) != -1) {
		switch (opt) {
		case 'a':
			tms->arfcn = atoi(optarg);
			break;
		case 't':
			pcap_file_path = strdup(optarg);
			break;
		case 'd':
			tms->dumpdir = strdup(optarg);
			break;
		case 'n':
			no_udp_tap = true;
			break;
		case 'u':
			tms->channel_type = TETRA_TYPE_UPLINK;
			break;
		case 'i':
			tms->channel_type = TETRA_TYPE_DIRECT;
			break;
		case 'c':
			tms->tcp.mcc = atoi(optarg);
			tms->tcp.mcnc_set = true;
			mcc_set = true;
			break;
		case 'm':
			tms->tcp.mnc = atoi(optarg);
			tms->tcp.mcnc_set = true;
			mnc_set = true;
			break;
		case 's':
			tms->tcp.colour_code = atoi(optarg);
			tms->tcp.cc_set = true;
			break;
		case 'h':
			hdist = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", opt);
			err = true;
		}
	}

	if (mcc_set != mnc_set) {
		fprintf(stderr, "You must set either both -m and -c or none of them!\n");
		err = true;
	}

	if (argc <= optind || err) {
		fprintf(stderr, "Usage: %s [params] <file_with_1_byte_per_bit>\n", argv[0]);
		fprintf(stderr, " -a arfcn     .. set ARFCN used in GSMTAP\n");
		fprintf(stderr, " -t filename  .. output PCAP to file\n");
		fprintf(stderr, " -d directory .. dump traffic channel to directory\n");
		fprintf(stderr, " -n           .. disable GSMTAP over UDP\n");
		fprintf(stderr, " -u           .. this is uplink (default: downlink)\n");
		fprintf(stderr, " -i           .. this is direct (default: downlink)\n");
		fprintf(stderr, " -c mcc       .. set MCC for uplink scrambling (mandatory)\n");
		fprintf(stderr, " -m mnc       .. set MNC for uplink scrambling (mandatory)\n");
		fprintf(stderr, " -s cc        .. set CC for uplink scrambling (will be automatically bruteforced if not provided)\n");
		fprintf(stderr, " -h num       .. set Hamming distance for training sequence search (default 0) (0 to 2 recommended) (slow)\n");
		exit(1);
	}

	if (tms->channel_type == TETRA_TYPE_UPLINK && !mcc_set ) {
		fprintf(stderr, "You have specified uplink, but not MCC and MNC. This is probably not what you want!\n");
	}

	fd = open(argv[optind], O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}

	if (!no_udp_tap) {
		tetra_gsmtap_init_network("localhost", 0);
	}
	if (pcap_file_path) {
		tetra_gsmtap_init_file(pcap_file_path);
	}

	int to_consume = BUFSIZE;
	uint8_t buf[BUFSIZE];

	while (1) {
		int len, rlen;

		len = read(fd, buf, to_consume);
		if (len < 0) {
			perror("read");
			exit(1);
		}
		rlen = len;
		if (len == 0) {
			memset(buf, 0, BUFSIZE);
			rlen = to_consume;
		}
		int rc = tetra_burst_sync_in(trs, buf, rlen, hdist);
		if (len == 0 && rc <= 0) {
			printf("EOF");
			break;
		}
		to_consume = MIN(abs(rc), BUFSIZE);
	}

	free(tms->dumpdir);
	talloc_free(trs);
	talloc_free(tms);

	exit(0);
}
