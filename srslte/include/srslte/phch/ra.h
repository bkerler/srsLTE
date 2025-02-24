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
 *  File:         ra.h
 *
 *  Description:  Structures and utility functions for DL/UL resource allocation.
 *                Convert an UL/DL unpacket DCI message to a resource allocation
 *
 *  Reference:    3GPP TS 36.213 version 10.0.1 Release 10
 *****************************************************************************/

#ifndef RB_ALLOC_H_
#define RB_ALLOC_H_

#include <stdint.h>
#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/common/phy_common.h"

/**************************************************
 * Common structures used for Resource Allocation 
 **************************************************/

typedef struct SRSLTE_API {
  srslte_mod_t mod;
  int tbs;
} srslte_ra_mcs_t;


/* Structure that gives the number of encoded bits and RE for a UL/DL grant */
typedef struct {
  uint32_t lstart; 
  uint32_t nof_symb; 
  uint32_t nof_bits;
  uint32_t nof_re; 
} srslte_ra_nbits_t;

typedef enum SRSLTE_API {
  SRSLTE_RA_ALLOC_TYPE0 = 0, 
  SRSLTE_RA_ALLOC_TYPE1 = 1, 
  SRSLTE_RA_ALLOC_TYPE2 = 2
} srslte_ra_type_t;

typedef struct SRSLTE_API {
  uint32_t rbg_bitmask;
} srslte_ra_type0_t;

typedef struct SRSLTE_API {
  uint32_t vrb_bitmask;
  uint32_t rbg_subset;
  bool shift;
} srslte_ra_type1_t;

typedef struct SRSLTE_API {
  uint32_t riv; // if L_crb==0, DCI message packer will take this value directly
  uint32_t L_crb;
  uint32_t RB_start;
  enum {
    SRSLTE_RA_TYPE2_NPRB1A_2 = 0, SRSLTE_RA_TYPE2_NPRB1A_3 = 1
  } n_prb1a;
  enum {
    SRSLTE_RA_TYPE2_NG1 = 0, SRSLTE_RA_TYPE2_NG2 = 1
  } n_gap;
  enum {
    SRSLTE_RA_TYPE2_LOC = 0, SRSLTE_RA_TYPE2_DIST = 1
  } mode;
} srslte_ra_type2_t;




/**************************************************
 * Structures used for Downlink Resource Allocation 
 **************************************************/

typedef struct SRSLTE_API {
  bool prb_idx[2][SRSLTE_MAX_PRB];
  uint32_t nof_prb;  
  uint32_t Qm; 
  srslte_ra_mcs_t mcs;
} srslte_ra_dl_grant_t;

/** Unpacked DCI message for DL grant */
typedef struct SRSLTE_API {
  
  enum {
    SRSLTE_RA_DCI_FORMAT1, 
    SRSLTE_RA_DCI_FORMAT1A, 
    SRSLTE_RA_DCI_FORMAT1C, 
  } dci_format; 
  
  srslte_ra_type_t alloc_type;
  union {
    srslte_ra_type0_t type0_alloc;
    srslte_ra_type1_t type1_alloc;
    srslte_ra_type2_t type2_alloc;
  };
  
  uint32_t mcs_idx;
  uint32_t harq_process;
  uint32_t rv_idx;
  bool ndi;
} srslte_ra_dl_dci_t;





/**************************************************
 * Structures used for Uplink Resource Allocation 
 **************************************************/

typedef struct SRSLTE_API {
  uint32_t n_prb[2];
  uint32_t n_prb_tilde[2];
  uint32_t L_prb;
  uint32_t freq_hopping; 
  uint32_t M_sc; 
  uint32_t M_sc_init; 
  uint32_t Qm; 
  srslte_ra_mcs_t mcs;
} srslte_ra_ul_grant_t;

/** Unpacked DCI Format0 message */
typedef struct SRSLTE_API {
  /* 36.213 Table 8.4-2: SRSLTE_RA_PUSCH_HOP_HALF is 0 for < 10 Mhz and 10 for > 10 Mhz.
   * SRSLTE_RA_PUSCH_HOP_QUART is 00 for > 10 Mhz and SRSLTE_RA_PUSCH_HOP_QUART_NEG is 01 for > 10 Mhz.
   */
  enum {
    SRSLTE_RA_PUSCH_HOP_DISABLED = -1,
    SRSLTE_RA_PUSCH_HOP_QUART = 0,
    SRSLTE_RA_PUSCH_HOP_QUART_NEG = 1,
    SRSLTE_RA_PUSCH_HOP_HALF = 2,
    SRSLTE_RA_PUSCH_HOP_TYPE2 = 3
  } freq_hop_fl;

  srslte_ra_ul_grant_t prb_alloc;

  srslte_ra_type2_t type2_alloc;
  uint32_t mcs_idx;
  uint32_t rv_idx;   
  uint32_t n_dmrs; 
  bool ndi;
  bool cqi_request;
  uint32_t tpc_pusch;

} srslte_ra_ul_dci_t;



/**************************************************
 * Functions 
 **************************************************/


SRSLTE_API int srslte_ra_dl_dci_to_grant(srslte_ra_dl_dci_t *dci, 
                                         uint32_t nof_prb, 
                                         bool crc_is_crnti, 
                                         srslte_ra_dl_grant_t *grant);

SRSLTE_API void srslte_ra_dl_grant_to_nbits(srslte_ra_dl_grant_t *grant, 
                                            uint32_t cfi, 
                                            srslte_cell_t cell, 
                                            uint32_t sf_idx, 
                                            srslte_ra_nbits_t *nbits); 

SRSLTE_API uint32_t srslte_ra_dl_grant_nof_re(srslte_ra_dl_grant_t *grant, 
                                              srslte_cell_t cell, 
                                              uint32_t sf_idx, 
                                              uint32_t nof_ctrl_symbols); 

SRSLTE_API int srslte_ra_ul_dci_to_grant(srslte_ra_ul_dci_t *dci, 
                                         uint32_t nof_prb,
                                         uint32_t n_rb_ho, 
                                         srslte_ra_ul_grant_t *grant);

SRSLTE_API void srslte_ra_ul_grant_to_nbits(srslte_ra_ul_grant_t *grant, 
                                            srslte_cp_t cp,
                                            uint32_t N_srs, 
                                            srslte_ra_nbits_t *nbits); 

SRSLTE_API int srslte_ul_dci_to_grant_prb_allocation(srslte_ra_ul_dci_t *dci, 
                                                     srslte_ra_ul_grant_t *grant,
                                                     uint32_t n_rb_ho, 
                                                     uint32_t nof_prb); 

SRSLTE_API int srslte_ra_tbs_from_idx(uint32_t tbs_idx, 
                                      uint32_t n_prb); 

SRSLTE_API int srslte_ra_tbs_to_table_idx(uint32_t tbs, 
                                          uint32_t n_prb);

SRSLTE_API uint32_t srslte_ra_type0_P(uint32_t nof_prb);

SRSLTE_API uint32_t srslte_ra_type2_to_riv(uint32_t L_crb, 
                                           uint32_t RB_start, 
                                           uint32_t nof_prb);

SRSLTE_API void srslte_ra_type2_from_riv(uint32_t riv, 
                                         uint32_t *L_crb, 
                                         uint32_t *RB_start,
                                         uint32_t nof_prb, 
                                         uint32_t nof_vrb);

SRSLTE_API uint32_t srslte_ra_type2_n_vrb_dl(uint32_t nof_prb, 
                                             bool ngap_is_1);

SRSLTE_API uint32_t srslte_ra_type2_n_rb_step(uint32_t nof_prb);

SRSLTE_API uint32_t srslte_ra_type2_ngap(uint32_t nof_prb, 
                                         bool ngap_is_1);

SRSLTE_API uint32_t srslte_ra_type1_N_rb(uint32_t nof_prb);

SRSLTE_API void srslte_ra_pdsch_fprint(FILE *f, 
                                       srslte_ra_dl_dci_t *ra, 
                                       uint32_t nof_prb);

SRSLTE_API void srslte_ra_dl_grant_fprint(FILE *f, 
                                          srslte_ra_dl_grant_t *grant);

SRSLTE_API void srslte_ra_prb_fprint(FILE *f, 
                                     srslte_ra_dl_grant_t *grant);

SRSLTE_API void srslte_ra_pusch_fprint(FILE *f, 
                                       srslte_ra_ul_dci_t *ra, 
                                       uint32_t nof_prb);

SRSLTE_API void srslte_ra_ul_grant_fprint(FILE *f, 
                                          srslte_ra_ul_grant_t *grant);

#endif /* RB_ALLOC_H_ */
