#ifndef TETRA_BURST_H
#define TETRA_BURST_H

#include <stdint.h>
#include "tetra_common.h"

enum tp_sap_data_type {
	TPSAP_T_SB1,
	TPSAP_T_SB2,
	TPSAP_T_NDB,
	TPSAP_T_BBK,
	TPSAP_T_SCH_HU,
	TPSAP_T_SCH_F,
};

extern void tp_sap_udata_ind(enum tp_sap_data_type type, const uint8_t *bits, unsigned int len, struct tetra_mac_state *priv);

/* 9.4.4.3.2 Normal Training Sequence */
static const uint8_t n_bits[22] = { 1,1, 0,1, 0,0, 0,0, 1,1, 1,0, 1,0, 0,1, 1,1, 0,1, 0,0 };
static const uint8_t p_bits[22] = { 0,1, 1,1, 1,0, 1,0, 0,1, 0,0, 0,0, 1,1, 0,1, 1,1, 1,0 };
static const uint8_t q_bits[22] = { 1,0, 1,1, 0,1, 1,1, 0,0, 0,0, 0,1, 1,0, 1,0, 1,1, 0,1 };
static const uint8_t N_bits[33] = { 1,1,1, 0,0,1, 1,0,1, 1,1,1, 0,0,0, 1,1,1, 1,0,0, 0,1,1, 1,1,0, 0,0,0, 0,0,0 };
static const uint8_t P_bits[33] = { 1,0,1, 0,1,1, 1,1,1, 1,0,1, 0,1,0, 1,0,1, 1,1,0, 0,0,1, 1,0,0, 0,1,0, 0,1,0 };

/* 9.4.4.3.3 Extended training sequence */
static const uint8_t x_bits[30] = { 1,0, 0,1, 1,1, 0,1, 0,0, 0,0, 1,1, 1,0, 1,0, 0,1, 1,1, 0,1, 0,0, 0,0, 1,1 };
static const uint8_t X_bits[45] = { 0,1,1,1,0,0,1,1,0,1,0,0,0,0,1,0,0,0,1,1,1,0,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,0,0,0,0,1,1,1,0 };

/* 9.4.4.3.4 Synchronization training sequence */
static const uint8_t y_bits[38] = { 1,1, 0,0, 0,0, 0,1, 1,0, 0,1, 1,1, 0,0, 1,1, 1,0, 1,0, 0,1, 1,1, 0,0, 0,0, 0,1, 1,0, 0,1, 1,1 };

/* 9.4.4.3.5 Tail bits */
static const uint8_t t_bits[4] = { 1, 1, 0, 0 };
static const uint8_t T_bits[6] = { 1, 1, 1, 0, 0, 0 };

/* 9.4.4.2.6 Synchronization continuous downlink burst */
int build_sync_c_d_burst(uint8_t *buf, const uint8_t *sb, const uint8_t *bb, const uint8_t *bkn);

/* 9.4.4.2.5 Normal continuous downlink burst */
int build_norm_c_d_burst(uint8_t *buf, const uint8_t *bkn1, const uint8_t *bb, const uint8_t *bkn2, int two_log_chan);

enum tetra_train_seq {
	TETRA_TRAIN_NORM_1,
	TETRA_TRAIN_NORM_2,
	TETRA_TRAIN_NORM_3,
	TETRA_TRAIN_SYNC,
	TETRA_TRAIN_EXT,
};

/* find a TETRA training sequence in the burst buffer indicated */
int tetra_find_train_seq(const uint8_t *in, unsigned int end_of_in,
			 uint32_t mask_of_train_seq, unsigned int *offset);

#endif /* TETRA_BURST_H */
