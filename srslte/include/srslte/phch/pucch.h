/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 The srsLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 *  File:         pucch.h
 *
 *  Description:  Physical uplink control channel.
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 5.4
 *****************************************************************************/

#ifndef PUCCH_
#define PUCCH_

#include "srslte/config.h"
#include "srslte/common/phy_common.h"
#include "srslte/common/sequence.h"
#include "srslte/modem/mod.h"

#define SRSLTE_PUCCH_N_SEQ       12 
#define SRSLTE_PUCCH_MAX_BITS    20
#define SRSLTE_PUCCH_MAX_SYMBOLS 120 

typedef enum SRSLTE_API {
  SRSLTE_PUCCH_FORMAT_1 = 0, 
  SRSLTE_PUCCH_FORMAT_1A, 
  SRSLTE_PUCCH_FORMAT_1B, 
  SRSLTE_PUCCH_FORMAT_2, 
  SRSLTE_PUCCH_FORMAT_2A, 
  SRSLTE_PUCCH_FORMAT_2B, 
} srslte_pucch_format_t; 

typedef struct SRSLTE_API {
  uint32_t n_cce; 
  bool sps_enabled; 
  uint32_t tpc_for_pucch; 
  uint32_t N_pucch_1; 
  uint32_t n_pucch_1[4]; // 4 n_pucch resources specified by RRC  
  uint32_t n_pucch_2; 
  uint32_t n_pucch_sr; 
}srslte_pucch_sched_t;

typedef struct SRSLTE_API {
  float beta_pucch;
  uint32_t delta_pucch_shift; 
  uint32_t n_rb_2; 
  uint32_t N_cs; 
  bool srs_configured; 
  uint32_t srs_cs_subf_cfg;
  bool srs_simul_ack; 
} srslte_pucch_cfg_t;

/* PUSCH object */
typedef struct SRSLTE_API {
  srslte_cell_t cell;
  srslte_pucch_cfg_t pucch_cfg;
  srslte_sequence_t seq_f2[SRSLTE_NSUBFRAMES_X_FRAME]; 
  srslte_modem_table_t mod; 
  
  uint8_t bits_scram[SRSLTE_PUCCH_MAX_BITS];
  cf_t d[SRSLTE_PUCCH_MAX_BITS/2];
  uint32_t n_cs_cell[SRSLTE_NSLOTS_X_FRAME][SRSLTE_CP_NORM_NSYMB]; 
  uint32_t f_gh[SRSLTE_NSLOTS_X_FRAME];
  float tmp_arg[SRSLTE_PUCCH_N_SEQ];
  cf_t z[SRSLTE_PUCCH_MAX_SYMBOLS];
  bool rnti_is_set;
  bool shortened; 
  bool group_hopping_en;
}srslte_pucch_t;


SRSLTE_API int srslte_pucch_init(srslte_pucch_t *q, 
                                 srslte_cell_t cell);

SRSLTE_API void srslte_pucch_free(srslte_pucch_t *q);

SRSLTE_API bool srslte_pucch_set_cfg(srslte_pucch_t* q, 
                                     srslte_pucch_cfg_t* cfg, 
                                     bool group_hopping_en); 

SRSLTE_API int srslte_pucch_set_crnti(srslte_pucch_t *q, 
                                      uint16_t c_rnti); 

SRSLTE_API uint32_t srslte_pucch_nof_symbols(srslte_pucch_cfg_t *cfg, 
                                             srslte_pucch_format_t format, 
                                             bool shortened); 

SRSLTE_API int srslte_pucch_encode(srslte_pucch_t *q, 
                                   srslte_pucch_format_t format,
                                   uint32_t n_pucch, // n_pucch_1 or n_pucch_2 depending on format
                                   uint32_t sf_idx, 
                                   uint8_t bits[SRSLTE_PUCCH_MAX_BITS], 
                                   cf_t *sf_symbols); 

SRSLTE_API float srslte_pucch_alpha_format1(uint32_t n_cs_cell[SRSLTE_NSLOTS_X_FRAME][SRSLTE_CP_NORM_NSYMB], 
                                            srslte_pucch_cfg_t *cfg, 
                                            uint32_t n_pucch, 
                                            srslte_cp_t cp, 
                                            bool is_dmrs,
                                            uint32_t ns, 
                                            uint32_t l,
                                            uint32_t *n_oc,
                                            uint32_t *n_prime_ns); 

SRSLTE_API float srslte_pucch_alpha_format2(uint32_t n_cs_cell[SRSLTE_NSLOTS_X_FRAME][SRSLTE_CP_NORM_NSYMB], 
                                            srslte_pucch_cfg_t *cfg, 
                                            uint32_t n_pucch, 
                                            uint32_t ns, 
                                            uint32_t l); 

SRSLTE_API uint32_t srslte_pucch_m(srslte_pucch_cfg_t *cfg, 
                                   srslte_pucch_format_t format, 
                                   uint32_t n_pucch, 
                                   srslte_cp_t cp); 

SRSLTE_API int srslte_pucch_n_cs_cell(srslte_cell_t cell, 
                                      uint32_t n_cs_cell[SRSLTE_NSLOTS_X_FRAME][SRSLTE_CP_NORM_NSYMB]);

SRSLTE_API int srslte_pucch_format2ab_mod_bits(srslte_pucch_format_t format, 
                                               uint8_t bits[2], 
                                               cf_t *d_10); 

SRSLTE_API bool srslte_pucch_cfg_isvalid(srslte_pucch_cfg_t *cfg, 
                                         uint32_t nof_prb); 

SRSLTE_API bool srslte_n_pucch_isvalid(srslte_pucch_t *q, 
                                       uint32_t n_pucch);

SRSLTE_API void srslte_pucch_cfg_default(srslte_pucch_cfg_t *cfg); 

#endif
