/* Implementation of TETRA burst synchronization */

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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <osmocom/core/utils.h>

//#define DEBUG

#include <tetra_common.h>
#include <phy/tetra_burst.h>
#include <tetra_tdma.h>
#include <phy/tetra_burst_sync.h>

struct tetra_phy_state t_phy_state;

void tetra_burst_rx_cb(const uint8_t *burst, unsigned int len, enum tetra_train_seq type, struct tetra_mac_state *tms);
void tetra_burst_rx_ul(const uint8_t *burst, unsigned int len, enum tetra_train_seq type, struct tetra_mac_state *tms);
void tetra_burst_rx_di(const uint8_t *burst, unsigned int len, enum tetra_train_seq type, struct tetra_mac_state *tms);

static unsigned int make_bitbuf_space(struct tetra_rx_state *trs, unsigned int len)
{
	unsigned int bitbuf_space = sizeof(trs->bitbuf) - trs->bits_in_buf;

	if (bitbuf_space < len) {
		unsigned int delta = len - bitbuf_space;

		DEBUGP("bitbuf left: %u, shrinking by %u\n", bitbuf_space, delta);
		memmove(trs->bitbuf, trs->bitbuf + delta, trs->bits_in_buf - delta);
		trs->bits_in_buf -= delta;
		trs->bitbuf_start_bitnum += delta;
		bitbuf_space = sizeof(trs->bitbuf) - trs->bits_in_buf;
	}
	return bitbuf_space;
}

static unsigned int conserve_bits(struct tetra_rx_state *trs, unsigned int howmany)
{
	assert(howmany < sizeof(trs->bitbuf));
	return make_bitbuf_space(trs, sizeof(trs->bitbuf) - howmany);
}

/* input a raw bitstream into the tetra burst synchronizer
 * returns the number which in absolute value is the number of bits
 *  we can safely consume the next time
 * if the result is negative, we are done with processing
 */
int tetra_burst_sync_in(struct tetra_rx_state *trs, uint8_t *bits, unsigned int len)
{
	int rc;
	unsigned int train_seq_offs;

	DEBUGP("burst_sync_in: %u bits, state %u\n", len, trs->state);

	/* First: append the data to the bitbuf */
	make_bitbuf_space(trs, len);
	memcpy(trs->bitbuf + trs->bits_in_buf, bits, len);
	trs->bits_in_buf += len;

	if (trs->burst_cb_priv->channel_type == TETRA_TYPE_DOWNLINK) {
		rc = tetra_find_train_seq(trs->bitbuf+214, trs->bits_in_buf,
					  (1 << TETRA_TRAIN_NORM_1)|
					  (1 << TETRA_TRAIN_NORM_2)|
					  (1 << TETRA_TRAIN_SYNC), &train_seq_offs);
		train_seq_offs += 214;

		if ((rc < 0) || (train_seq_offs + TETRA_BITS_PER_TS > trs->bits_in_buf)) {
			return -conserve_bits(trs, 2*TETRA_BITS_PER_TS);
		}

		tetra_tdma_time_add_tn(&t_phy_state.time, 1);
		printf("\nBURST @ %u", trs->bitbuf_start_bitnum+train_seq_offs);
		DEBUGP(": %s", osmo_ubit_dump(trs->bitbuf, TETRA_BITS_PER_TS));
		printf("\n");

		switch (rc) {
		case TETRA_TRAIN_SYNC:
			if (train_seq_offs >= 214) {
				tetra_burst_rx_cb(trs->bitbuf+train_seq_offs-214, TETRA_BITS_PER_TS, rc, trs->burst_cb_priv);
			}
			break;
		case TETRA_TRAIN_NORM_1:
		case TETRA_TRAIN_NORM_2:
		case TETRA_TRAIN_NORM_3:
			if (train_seq_offs >= 244) {
				tetra_burst_rx_cb(trs->bitbuf+train_seq_offs-244, TETRA_BITS_PER_TS, rc, trs->burst_cb_priv);
			}
			break;
		default:
			fprintf(stderr, "#### unsupported burst training sequence\n");
			break;
		}
		return train_seq_offs+1;
	} else if (trs->burst_cb_priv->channel_type == TETRA_TYPE_UPLINK) {

		rc = tetra_find_train_seq(trs->bitbuf+122, trs->bits_in_buf,
					(1 << TETRA_TRAIN_NORM_1)|
					(1 << TETRA_TRAIN_NORM_2)|
					(1 << TETRA_TRAIN_EXT), &train_seq_offs);
		train_seq_offs += 122;

		if ((rc < 0) || (train_seq_offs + TETRA_BITS_PER_TS > trs->bits_in_buf)) {
			return -conserve_bits(trs, 2*TETRA_BITS_PER_TS);
		}
		switch (rc) {
		case TETRA_TRAIN_EXT:
			if (train_seq_offs >= 122) {
				tetra_burst_rx_ul(trs->bitbuf+train_seq_offs-122, TETRA_BITS_PER_TS/2, rc, trs->burst_cb_priv);
			}
			break;
		case TETRA_TRAIN_NORM_1:
			if (train_seq_offs >= 254) {
				tetra_burst_rx_ul(trs->bitbuf+train_seq_offs-254, TETRA_BITS_PER_TS, rc, trs->burst_cb_priv);
			}
			break;
		case TETRA_TRAIN_NORM_2:
			DEBUGP("TETRA_TRAIN_NORM_2, ignore\n");
			break;
		}
		return train_seq_offs+1;
	} else if (trs->burst_cb_priv->channel_type == TETRA_TYPE_DIRECT) {
		rc = tetra_find_train_seq(trs->bitbuf+214, trs->bits_in_buf,
					(1 << TETRA_TRAIN_SYNC)|
					(1 << TETRA_TRAIN_NORM_1), &train_seq_offs);
		train_seq_offs += 214;

		if ((rc < 0) || (train_seq_offs + TETRA_BITS_PER_TS > trs->bits_in_buf)) {
			return -conserve_bits(trs, 2*TETRA_BITS_PER_TS);
		}

		switch (rc) {
		case TETRA_TRAIN_NORM_1:
			if (train_seq_offs >= 230) {
				tetra_burst_rx_di(trs->bitbuf+train_seq_offs-230, TETRA_BITS_PER_TS, rc, trs->burst_cb_priv);
			}
			break;
		case TETRA_TRAIN_SYNC:
			if (train_seq_offs >= 214) {
				tetra_burst_rx_di(trs->bitbuf+train_seq_offs-214, TETRA_BITS_PER_TS, rc, trs->burst_cb_priv);
			}
			break;
		}
		return train_seq_offs+1;
	}

	assert(0);
}
