/*! \file auth_xor.c
 * XOR algorithm as per 34.108 Section 8.1.2 */
/*
 * (C) 2018 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <string.h>

#include <osmocom/crypt/auth.h>
#include <osmocom/core/bits.h>

/*! \addtogroup auth
 *  @{
 */

static void xor_n(uint8_t *out, const uint8_t *a, const uint8_t *b, unsigned int num_bytes)
{
	unsigned int i;
	for (i = 0; i < num_bytes; i++)
		out[i] = a[i] ^ b[i];
}
#define xor16(out, a, b)	xor_n(out, a, b, 16)
#define xor8(out, a, b)		xor_n(out, a, b, 8)
#define xor6(out, a, b)		xor_n(out, a, b, 6)

/* TS 34.108 Section 8.1.2.1 */
static int xor_gen_vec(struct osmo_auth_vector *vec,
			  struct osmo_sub_auth_data *aud,
			  const uint8_t *_rand)
{
	uint8_t xdout[16];
	uint8_t ak[6];
	uint8_t cdout[8];
	uint8_t xmac[8];
	uint8_t sqn[6];
	uint8_t sqn_ak[6];
	unsigned int i;

	osmo_store64be_ext(aud->u.umts.sqn, sqn, 6);

	/* Step 1: XOR to the challenge RAND a predefined number K (in which at least one bit is not
	 * zero, see clause 8.2), having the same length (128 bits) as RAND */
	xor16(xdout, _rand, aud->u.umts.k);

	/* Step 2: RES[bits 0,1,...n-1,n] = f2(XDOUT,n) = XDOUT[bits 0,1,...n-1,n] */
	vec->res_len = sizeof(vec->res);
	memcpy(vec->res, xdout, vec->res_len);

	/* CK[bits 0,1,...126,127] = f3(XDOUT) = XDOUT[bits 8,9,...126,127,0,1,...6,7]  */
	memcpy(vec->ck, xdout+1, 15);
	memcpy(vec->ck+15, xdout, 1);

	/* IK[bits 0,1,...126,127] = f4(XDOUT) = XDOUT[bits 16,17,...126,127,0,1,...14,15]  */
	memcpy(vec->ik, xdout+2, 14);
	memcpy(vec->ik+14, xdout, 2);

	/* AK[bits 0,1,...46,47] = f5(XDOUT) = XDOUT[bits 24,25,...70,71] */
	memcpy(ak, xdout+3, 6);

	/* Kc[bits 0,1,...62,63] = c3(CK,IK), see 3GPP TS 33.102 [24], clause 6.8.1.2. */
	osmo_auth_c3(vec->kc, vec->ck, vec->ik);

	/* not really stated in TS 34.108 !?! */
	for (i = 0; i < 4; i++)
		vec->sres[i] = vec->res[i] ^ vec->res[i + 4];

	/* Step 3: Concatenate SQN with AMF to obtain CDOUT like this:
	 * CDOUT[bits 0,1,...62,63] = SQN[bits 0,1,...46,47] || AMF[bits 0,1,...14,15]
	 * For test USIM the SQN = SQNMS = SQNSS[bits 0,1,...46,47] = 
	 * 				AUTN[bits 0,1,...46,47] XOR AK[bits 0,1, ... 46,47]
	 * where AUTN is the received authentication token. */
	memcpy(cdout, sqn, 6);
	memcpy(cdout+6, aud->u.umts.amf, 2);

	/* Step 4: XMAC (test USIM) and MAC (SS) are calculated from XDOUT and CDOUT this way:
	 * XMAC[bits 0,1,...62, 63] = f1(XDOUT, CDOUT) =
	 * 				XDOUT[bits 0,1...62,63] XOR CDOUT[bits 0,1,...62,63]
	 * NOTE: In SS and AUC, the MAC calculation is identical to XMAC. */
	xor8(xmac, xdout, cdout);

	/* Step 5: The SS calculates the authentication token AUTN:
	 * AUTN[bits 0,1,..126,127] = SQN ⊕ AK[bits 0,1,...46,47] || AMF[bits 0,1,...14,15] || MAC[bits 0,1,...62, 63]
	 * Where SQN ⊕ AK[bits 0,1,. . .46,47] = SQN[bits 0,1,. . .46,47] XOR AK[bits 0,1, . . . 46,47] */
	xor6(sqn_ak, sqn, ak);
	memcpy(vec->autn, sqn_ak, 6);
	memcpy(vec->autn+6, aud->u.umts.amf, 2);
	memcpy(vec->autn+8, xmac, 8);

	vec->auth_types = OSMO_AUTH_TYPE_GSM | OSMO_AUTH_TYPE_UMTS;

	return 0;
}


/* TS 34.108 Section 8.1.2.2 */
static int xor_gen_vec_auts(struct osmo_auth_vector *vec,
			    struct osmo_sub_auth_data *aud,
			    const uint8_t *auts, const uint8_t *rand_auts,
			    const uint8_t *_rand)
{
	uint8_t xdout[16];
	uint8_t ak[6];
	uint8_t sqnms[6];
	uint8_t cdout[8];
	uint8_t xmac_s[8];

	/* Step 1: XOR to the challenge RAND a predefined number K (in which at least one bit is not
	 * zero, see clause 8.2), having the same length (128 bits) as RAND */
	xor16(xdout, rand_auts, aud->u.umts.k);

	/* Step 2: AK is extracted from XDOUT this way:
	 * AK[bits 0,1,...46,47] = f5*(XDOUT) = XDOUT[bits 24,25,...70,71] */
	memcpy(ak, xdout+3, 6);

	/* For SS and AUC the SQNMS = AUTS[bits 0,1,...46,47] XOR AK[bits 0,1,...46,47] where
	 * AUTS is the received re-synchronization parameter. */
	xor6(sqnms, auts, ak);

	/* Concatenate SQNMS with AMF* to obtain CDOUT like this:
	 * CDOUT[bits 0,1,. . .62,63] = SQNMS[bits 0,1,. . .46,47] || AMF*[bits 0,1,. . .14,15]
	 * Where AMF* assumes a dummy value of all zeros. */
	memcpy(cdout, sqnms, 6);
	memset(cdout+6, 0, 2);

	/* MAC-S[bits 0,1,...62, 63] = f1*(XDOUT, CDOUT) =
	 * 				XDOUT[bits 0,1...62,63] XOR CDOUT[bits 0,1,...62,63]
	 * In SS and AUC, the XMAC-S calculation is identical to MAC-S. */
	xor8(xmac_s, xdout, cdout);

	/* The test USIM calculates the re-synchronization parameter AUTS:
	 * AUTS[bits 0,1,..110,111] = SQNMS ⊕ AK[bits 0,1,. . .46,47] || MAC-S[bits 0,1,...62, 63]
	 * Where SQNMS ⊕ AK[bits 0,1,...46,47] = SQNMS [bits 0,1,...46,47] XOR AK[bits 0,1,...46,47]
	 */

	/* compare the last 64bit of received AUTS wit the locally-generated MAC-S */
	if (memcmp(auts+6, xmac_s, 8))
		return -1;

	aud->u.umts.sqn_ms = osmo_load64be_ext(sqnms, 6) >> 16;
	/* Update our "largest used SQN" from the USIM -- milenage_gen_vec()
	 * below will increment SQN. */
	aud->u.umts.sqn = aud->u.umts.sqn_ms;

	return xor_gen_vec(vec, aud, _rand);
}


static struct osmo_auth_impl xor_alg = {
	.algo = OSMO_AUTH_ALG_XOR,
	.name = "XOR (libosmogsm built-in)",
	.priority = 1000,
	.gen_vec = &xor_gen_vec,
	.gen_vec_auts = &xor_gen_vec_auts,
};

static __attribute__((constructor)) void on_dso_load_xor(void)
{
	osmo_auth_register(&xor_alg);
}

/*! @} */
