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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "srslte/common/phy_common.h"
#include "srslte/utils/bit.h"
#include "srslte/utils/vector.h"
#include "srslte/utils/debug.h"
#include "srslte/phch/ra.h"
#include "srslte/utils/bit.h"

#include "tbs_tables.h"

#define min(a,b) (a<b?a:b)

/* Returns the number of RE in a PRB in a slot and subframe */
uint32_t ra_re_x_prb(uint32_t subframe, uint32_t slot, uint32_t prb_idx, uint32_t nof_prb,
    uint32_t nof_ports, uint32_t nof_ctrl_symbols, srslte_cp_t cp) {

  uint32_t re;
  bool skip_refs = false;

  if (slot == 0) {
    re = (SRSLTE_CP_NSYMB(cp) - nof_ctrl_symbols) * SRSLTE_NRE;
  } else {
    re = SRSLTE_CP_NSYMB(cp) * SRSLTE_NRE;
  }

  /* if it's the prb in the middle, there are less RE due to PBCH and PSS/SSS */
  if ((subframe == 0 || subframe == 5)
      && (prb_idx >= nof_prb / 2 - 3 && prb_idx <= nof_prb / 2 + 3)) {
    if (subframe == 0) {
      if (slot == 0) {
        re = (SRSLTE_CP_NSYMB(cp) - nof_ctrl_symbols - 2) * SRSLTE_NRE;
      } else {
        if (SRSLTE_CP_ISEXT(cp)) {
          re = (SRSLTE_CP_NSYMB(cp) - 4) * SRSLTE_NRE;
          skip_refs = true;
        } else {
          re = (SRSLTE_CP_NSYMB(cp) - 4) * SRSLTE_NRE + 2 * nof_ports;
        }
      }
    } else if (subframe == 5) {
      if (slot == 0) {
        re = (SRSLTE_CP_NSYMB(cp) - nof_ctrl_symbols - 2) * SRSLTE_NRE;
      }
    }
    if ((nof_prb % 2)
        && (prb_idx == nof_prb / 2 - 3 || prb_idx == nof_prb / 2 + 3)) {
      if (slot == 0) {
        re += 2 * SRSLTE_NRE / 2;
      } else if (subframe == 0) {
        re += 4 * SRSLTE_NRE / 2 - nof_ports;
        if (SRSLTE_CP_ISEXT(cp)) {
          re -= nof_ports > 2 ? 2 : nof_ports;
        }
      }
    }
  }

  // remove references
  if (!skip_refs) {
    switch (nof_ports) {
    case 1:
    case 2:
      re -= 2 * (slot + 1) * nof_ports;
      break;
    case 4:
      if (slot == 1) {
        re -= 12;
      } else {
        re -= 4;
        if (nof_ctrl_symbols == 1) {
          re -= 4;
        }
      }
      break;
    }
  }

  return re;
}

int srslte_ul_dci_to_grant_prb_allocation(srslte_ra_ul_dci_t *dci, srslte_ra_ul_grant_t *grant, uint32_t n_rb_ho, uint32_t nof_prb) 
{
  bzero(grant, sizeof(srslte_ra_ul_grant_t));  
  grant->L_prb = dci->type2_alloc.L_crb;
  uint32_t n_prb_1 = dci->type2_alloc.RB_start;
  uint32_t n_rb_pusch = 0;

  if (n_rb_ho%2) {
    n_rb_ho++;
  }

  if (dci->freq_hop_fl == SRSLTE_RA_PUSCH_HOP_DISABLED || dci->freq_hop_fl == SRSLTE_RA_PUSCH_HOP_TYPE2) {
    /* For no freq hopping or type2 freq hopping, n_prb is the same 
     * n_prb_tilde is calculated during resource mapping
     */
    for (uint32_t i=0;i<2;i++) {
      grant->n_prb[i] = n_prb_1;        
    }
    if (dci->freq_hop_fl == SRSLTE_RA_PUSCH_HOP_DISABLED) {
      grant->freq_hopping = 0;
    } else {
      grant->freq_hopping = 2;      
    }
    INFO("prb1: %d, prb2: %d, L: %d\n", grant->n_prb[0], grant->n_prb[1], grant->L_prb);
  } else {
    /* Type1 frequency hopping as defined in 8.4.1 of 36.213 
      * frequency offset between 1st and 2nd slot is fixed. 
      */
    n_rb_pusch = nof_prb - n_rb_ho - (nof_prb%2);
    
    // starting prb idx for slot 0 is as given by resource grant
    grant->n_prb[0] = n_prb_1;
    if (n_prb_1 < n_rb_ho/2) {
      fprintf(stderr, "Invalid Frequency Hopping parameters. Offset: %d, n_prb_1: %d\n", n_rb_ho, n_prb_1);
    }
    uint32_t n_prb_1_tilde = n_prb_1;

    // prb idx for slot 1 
    switch(dci->freq_hop_fl) {
      case SRSLTE_RA_PUSCH_HOP_QUART:
        grant->n_prb[1] = (n_rb_pusch/4+ n_prb_1_tilde)%n_rb_pusch;            
        break;
      case SRSLTE_RA_PUSCH_HOP_QUART_NEG:
        if (n_prb_1 < n_rb_pusch/4) {
          grant->n_prb[1] = (n_rb_pusch+ n_prb_1_tilde -n_rb_pusch/4);                                
        } else {
          grant->n_prb[1] = (n_prb_1_tilde -n_rb_pusch/4);                      
        }
        break;
      case SRSLTE_RA_PUSCH_HOP_HALF:
        grant->n_prb[1] = (n_rb_pusch/2+ n_prb_1_tilde)%n_rb_pusch;            
        break;
      default:
        break;        
    }
    INFO("n_rb_pusch: %d, prb1: %d, prb2: %d, L: %d\n", n_rb_pusch, grant->n_prb[0], grant->n_prb[1], grant->L_prb);
    grant->freq_hopping = 1;
  }
  return SRSLTE_SUCCESS; 
}

static int ul_dci_to_grant_mcs(srslte_ra_ul_dci_t *dci, srslte_ra_ul_grant_t *grant) {  
  int tbs = -1; 
  // 8.6.2 First paragraph
  if (dci->mcs_idx <= 28) {
    /* Table 8.6.1-1 on 36.213 */
    if (dci->mcs_idx < 11) {
      grant->mcs.mod = SRSLTE_MOD_QPSK;
      tbs = srslte_ra_tbs_from_idx(dci->mcs_idx, grant->L_prb);      
    } else if (dci->mcs_idx < 21) {
      grant->mcs.mod = SRSLTE_MOD_16QAM;
      tbs = srslte_ra_tbs_from_idx(dci->mcs_idx - 1, grant->L_prb);
    } else if (dci->mcs_idx < 29) {
      grant->mcs.mod = SRSLTE_MOD_64QAM;
      tbs = srslte_ra_tbs_from_idx(dci->mcs_idx - 2, grant->L_prb);
    } else {
      fprintf(stderr, "Invalid MCS index %d\n", dci->mcs_idx);
    }
  } else if (dci->mcs_idx == 29 && dci->cqi_request && grant->L_prb <= 4) {
    // 8.6.1 and 8.6.2 36.213 second paragraph
    grant->mcs.mod = SRSLTE_MOD_QPSK;
    tbs = 0;
  } else if (dci->mcs_idx >= 29) {
    // Else use last TBS/Modulation and use mcs to obtain rv_idx 
    tbs = 0; 
    grant->mcs.mod = 0; 
    dci->rv_idx = dci->mcs_idx - 28;
  }
  if (tbs < 0) {
    fprintf(stderr, "Error computing TBS\n");
    return SRSLTE_ERROR; 
  } else {
    grant->mcs.tbs = (uint32_t) tbs; 
    return SRSLTE_SUCCESS;
  }
}

void srslte_ra_ul_grant_to_nbits(srslte_ra_ul_grant_t *grant, srslte_cp_t cp, uint32_t N_srs, srslte_ra_nbits_t *nbits) 
{
  nbits->nof_symb = 2*(SRSLTE_CP_NSYMB(cp)-1) - N_srs; 
  nbits->nof_re   = nbits->nof_symb*grant->M_sc;
  nbits->nof_bits = nbits->nof_re * grant->Qm;
}

/** Compute PRB allocation for Uplink as defined in 8.1 and 8.4 of 36.213 */
int srslte_ra_ul_dci_to_grant(srslte_ra_ul_dci_t *dci, uint32_t nof_prb, uint32_t n_rb_ho, srslte_ra_ul_grant_t *grant) 
{
  
  // Compute PRB allocation 
  if (!srslte_ul_dci_to_grant_prb_allocation(dci, grant, n_rb_ho, nof_prb)) {
    
    // Compute MCS 
    if (!ul_dci_to_grant_mcs(dci, grant)) {
      
      // Fill rest of grant structure 
      grant->M_sc = grant->L_prb*SRSLTE_NRE;
      grant->M_sc_init = grant->M_sc; // FIXME: What should M_sc_init be? 
      grant->Qm = srslte_mod_bits_x_symbol(grant->mcs.mod);
    } else {
      fprintf(stderr, "Error computing MCS\n");
      return SRSLTE_ERROR; 
    }
  } else {
    fprintf(stderr, "Error computing PRB allocation\n");
    return SRSLTE_ERROR; 
  }
  return SRSLTE_SUCCESS;
}

/* Computes the number of RE for each PRB in the prb_dist structure */
uint32_t srslte_ra_dl_grant_nof_re(srslte_ra_dl_grant_t *grant, srslte_cell_t cell, 
                                      uint32_t sf_idx, uint32_t nof_ctrl_symbols) 
{
  uint32_t j, s;

  // Compute number of RE per PRB
  uint32_t nof_re = 0;
  for (s = 0; s < 2; s++) {
    for (j = 0; j < cell.nof_prb; j++) {
      if (grant->prb_idx[s][j]) {
        nof_re += ra_re_x_prb(sf_idx, s, j,
            cell.nof_prb, cell.nof_ports, nof_ctrl_symbols, cell.cp);          
      }
    }
  }  
  return nof_re; 
}


/** Compute PRB allocation for Downlink as defined in 7.1.6 of 36.213 */
static int dl_dci_to_grant_prb_allocation(srslte_ra_dl_dci_t *dci, srslte_ra_dl_grant_t *grant, uint32_t nof_prb) {
  int i, j;
  uint32_t bitmask;
  uint32_t P = srslte_ra_type0_P(nof_prb);
  uint32_t n_rb_rbg_subset, n_rb_type1;
  
  bzero(grant, sizeof(srslte_ra_dl_grant_t));
  switch (dci->alloc_type) {
  case SRSLTE_RA_ALLOC_TYPE0:
    bitmask = dci->type0_alloc.rbg_bitmask;
    int nb = (int) ceilf((float) nof_prb / P);
    for (i = 0; i < nb; i++) {
      if (bitmask & (1 << (nb - i - 1))) {
        for (j = 0; j < P; j++) {
          if (i*P+j < nof_prb) {
            grant->prb_idx[0][i * P + j] = true;
            grant->nof_prb++;
          }
        }
      }
    }
    memcpy(&grant->prb_idx[1], &grant->prb_idx[0], SRSLTE_MAX_PRB*sizeof(bool));
    break;
  case SRSLTE_RA_ALLOC_TYPE1:
    n_rb_type1 = srslte_ra_type1_N_rb(nof_prb);
    if (dci->type1_alloc.rbg_subset < (nof_prb / P) % P) {
      n_rb_rbg_subset = ((nof_prb - 1) / (P * P)) * P + P;
    } else if (dci->type1_alloc.rbg_subset == ((nof_prb / P) % P)) {
      n_rb_rbg_subset = ((nof_prb - 1) / (P * P)) * P + ((nof_prb - 1) % P) + 1;
    } else {
      n_rb_rbg_subset = ((nof_prb - 1) / (P * P)) * P;
    }
    int shift = dci->type1_alloc.shift ? (n_rb_rbg_subset - n_rb_type1) : 0;
    bitmask = dci->type1_alloc.vrb_bitmask;
    for (i = 0; i < n_rb_type1; i++) {
      if (bitmask & (1 << (n_rb_type1 - i - 1))) {
        grant->prb_idx[0][((i + shift) / P)
            * P * P + dci->type1_alloc.rbg_subset * P + (i + shift) % P] = true;
        grant->nof_prb++;
      }
    }
    memcpy(&grant->prb_idx[1], &grant->prb_idx[0], SRSLTE_MAX_PRB*sizeof(bool));
    break;
  case SRSLTE_RA_ALLOC_TYPE2:
    if (dci->type2_alloc.mode == SRSLTE_RA_TYPE2_LOC) {
      for (i = 0; i < dci->type2_alloc.L_crb; i++) {
        grant->prb_idx[0][i + dci->type2_alloc.RB_start] = true;
        grant->nof_prb++;
      }
      memcpy(&grant->prb_idx[1], &grant->prb_idx[0], SRSLTE_MAX_PRB*sizeof(bool));
    } else {
      /* Mapping of Virtual to Physical RB for distributed type is defined in
       * 6.2.3.2 of 36.211
       */
      int N_gap, N_tilde_vrb, n_tilde_vrb, n_tilde_prb, n_tilde2_prb, N_null,
          N_row, n_vrb;
      int n_tilde_prb_odd, n_tilde_prb_even;
      if (dci->type2_alloc.n_gap == SRSLTE_RA_TYPE2_NG1) {
        N_tilde_vrb = srslte_ra_type2_n_vrb_dl(nof_prb, true);
        N_gap = srslte_ra_type2_ngap(nof_prb, true);
      } else {
        N_tilde_vrb = 2 * srslte_ra_type2_n_vrb_dl(nof_prb, true);
        N_gap = srslte_ra_type2_ngap(nof_prb, false);
      }
      N_row = (int) ceilf((float) N_tilde_vrb / (4 * P)) * P;
      N_null = 4 * N_row - N_tilde_vrb;
      for (i = 0; i < dci->type2_alloc.L_crb; i++) {
        n_vrb = i + dci->type2_alloc.RB_start;
        n_tilde_vrb = n_vrb % N_tilde_vrb;
        n_tilde_prb = 2 * N_row * (n_tilde_vrb % 2) + n_tilde_vrb / 2
            + N_tilde_vrb * (n_vrb / N_tilde_vrb);
        n_tilde2_prb = N_row * (n_tilde_vrb % 4) + n_tilde_vrb / 4
            + N_tilde_vrb * (n_vrb / N_tilde_vrb);

        if (N_null != 0 && n_tilde_vrb >= (N_tilde_vrb - N_null)
            && (n_tilde_vrb % 2) == 1) {
          n_tilde_prb_odd = n_tilde_prb - N_row;
        } else if (N_null != 0 && n_tilde_vrb >= (N_tilde_vrb - N_null)
            && (n_tilde_vrb % 2) == 0) {
          n_tilde_prb_odd = n_tilde_prb - N_row + N_null / 2;
        } else if (N_null != 0 && n_tilde_vrb < (N_tilde_vrb - N_null)
            && (n_tilde_vrb % 4) >= 2) {
          n_tilde_prb_odd = n_tilde2_prb - N_null / 2;
        } else {
          n_tilde_prb_odd = n_tilde2_prb;
        }
        n_tilde_prb_even = (n_tilde_prb_odd + N_tilde_vrb / 2) % N_tilde_vrb
            + N_tilde_vrb * (n_vrb / N_tilde_vrb);

        if (n_tilde_prb_odd < N_tilde_vrb / 2) {
          grant->prb_idx[0][n_tilde_prb_odd] = true;
        } else {
          grant->prb_idx[0][n_tilde_prb_odd + N_gap
              - N_tilde_vrb / 2] = true;
        }
        grant->nof_prb++;
        if (n_tilde_prb_even < N_tilde_vrb / 2) {
          grant->prb_idx[1][n_tilde_prb_even] = true;
        } else {
          grant->prb_idx[1][n_tilde_prb_even + N_gap
              - N_tilde_vrb / 2] = true;
        }
      }
    }
    break;
  default:
    return SRSLTE_ERROR;
  }

  return SRSLTE_SUCCESS;
}

/* Modulation order and transport block size determination 7.1.7 in 36.213 */
static int dl_dci_to_grant_mcs(srslte_ra_dl_dci_t *dci, srslte_ra_dl_grant_t *grant, bool crc_is_crnti) {
  uint32_t n_prb=0;
  int tbs = -1; 
  uint32_t i_tbs = 0; 
  
  if (!crc_is_crnti) {
    if (dci->dci_format == SRSLTE_RA_DCI_FORMAT1A) {
      n_prb = dci->type2_alloc.n_prb1a == SRSLTE_RA_TYPE2_NPRB1A_2 ? 2 : 3;
      i_tbs = dci->mcs_idx;
    } else {
      if (dci->mcs_idx < 32) {
        tbs = tbs_format1c_table[dci->mcs_idx];
      } 
    }
    grant->mcs.mod = SRSLTE_MOD_QPSK;      
  } else {
    n_prb = grant->nof_prb;
    if (dci->mcs_idx < 10) {
      grant->mcs.mod = SRSLTE_MOD_QPSK;
      i_tbs = dci->mcs_idx;
    } else if (dci->mcs_idx < 17) {
      grant->mcs.mod = SRSLTE_MOD_16QAM;
      i_tbs = dci->mcs_idx-1;
    } else if (dci->mcs_idx < 29) {
      grant->mcs.mod = SRSLTE_MOD_64QAM;
      i_tbs = dci->mcs_idx-2;
    } else if (dci->mcs_idx == 29) {
      grant->mcs.mod = SRSLTE_MOD_QPSK;
      tbs = 0;
      i_tbs = 0;
    } else if (dci->mcs_idx == 30) {
      grant->mcs.mod = SRSLTE_MOD_16QAM;
      tbs = 0;
      i_tbs = 0;
    } else if (dci->mcs_idx == 31) {
      grant->mcs.mod = SRSLTE_MOD_64QAM;
      tbs = 0;
      i_tbs = 0;
    }
  }  
  tbs = srslte_ra_tbs_from_idx(i_tbs, n_prb);
  
  if (tbs < 0) {
    return SRSLTE_ERROR; 
  } else {
    grant->mcs.tbs = (uint32_t) tbs; 
    return SRSLTE_SUCCESS; 
  }
}

void srslte_ra_dl_grant_to_nbits(srslte_ra_dl_grant_t *grant, uint32_t cfi, srslte_cell_t cell, uint32_t sf_idx, srslte_ra_nbits_t *nbits) 
{
  // Compute number of RE 
  nbits->nof_re = srslte_ra_dl_grant_nof_re(grant, cell, sf_idx, cell.nof_prb<10?(cfi+1):cfi);
  nbits->lstart = cell.nof_prb<10?(cfi+1):cfi;
  nbits->nof_symb = 2*SRSLTE_CP_NSYMB(cell.cp)-nbits->lstart;
  nbits->nof_bits = nbits->nof_re * grant->Qm;      
}

/** Obtains a DL grant from a DCI grant for PDSCH */
int srslte_ra_dl_dci_to_grant(srslte_ra_dl_dci_t *dci, uint32_t nof_prb, bool crc_is_crnti, srslte_ra_dl_grant_t *grant) 
{  
  // Compute PRB allocation 
  if (!dl_dci_to_grant_prb_allocation(dci, grant, nof_prb)) {
    // Compute MCS 
    if (!dl_dci_to_grant_mcs(dci, grant, crc_is_crnti)) {      
      // Fill rest of grant structure 
      grant->Qm = srslte_mod_bits_x_symbol(grant->mcs.mod);      
    } else {
      return SRSLTE_ERROR; 
    }
  } else {
    return SRSLTE_ERROR; 
  }
  return SRSLTE_SUCCESS;
}

/* RBG size for type0 scheduling as in table 7.1.6.1-1 of 36.213 */
uint32_t srslte_ra_type0_P(uint32_t nof_prb) {
  if (nof_prb <= 10) {
    return 1;
  } else if (nof_prb <= 26) {
    return 2;
  } else if (nof_prb <= 63) {
    return 3;
  } else {
    return 4;
  }
}

/* Returns N_rb_type1 according to section 7.1.6.2 */
uint32_t srslte_ra_type1_N_rb(uint32_t nof_prb) {
  uint32_t P = srslte_ra_type0_P(nof_prb);
  return (uint32_t) ceilf((float) nof_prb / P) - (uint32_t) ceilf(log2f((float) P)) - 1;
}

/* Convert Type2 scheduling L_crb and RB_start to RIV value */
uint32_t srslte_ra_type2_to_riv(uint32_t L_crb, uint32_t RB_start, uint32_t nof_prb) {
  uint32_t riv;
  if (L_crb <= nof_prb / 2) {
    riv = nof_prb * (L_crb - 1) + RB_start;
  } else {
    riv = nof_prb * (nof_prb - L_crb + 1) + nof_prb - 1 - RB_start;
  }
  return riv;
}

/* Convert Type2 scheduling RIV value to L_crb and RB_start values */
void srslte_ra_type2_from_riv(uint32_t riv, uint32_t *L_crb, uint32_t *RB_start,
    uint32_t nof_prb, uint32_t nof_vrb) {
  *L_crb = (uint32_t) (riv / nof_prb) + 1;
  *RB_start = (uint32_t) (riv % nof_prb);
  if (*L_crb > nof_vrb - *RB_start) {
    *L_crb = nof_prb - (int) (riv / nof_prb) + 1;
    *RB_start = nof_prb - riv % nof_prb - 1;
  }
}

/* Table 6.2.3.2-1 in 36.211 */
uint32_t srslte_ra_type2_ngap(uint32_t nof_prb, bool ngap_is_1) {
  if (nof_prb <= 10) {
    return nof_prb / 2;
  } else if (nof_prb == 11) {
    return 4;
  } else if (nof_prb <= 19) {
    return 8;
  } else if (nof_prb <= 26) {
    return 12;
  } else if (nof_prb <= 44) {
    return 18;
  } else if (nof_prb <= 49) {
    return 27;
  } else if (nof_prb <= 63) {
    return ngap_is_1 ? 27 : 9;
  } else if (nof_prb <= 79) {
    return ngap_is_1 ? 32 : 16;
  } else {
    return ngap_is_1 ? 48 : 16;
  }
}

/* Table 7.1.6.3-1 in 36.213 */
uint32_t srslte_ra_type2_n_rb_step(uint32_t nof_prb) {
  if (nof_prb < 50) {
    return 2;
  } else {
    return 4;
  }
}

/* as defined in 6.2.3.2 of 36.211 */
uint32_t srslte_ra_type2_n_vrb_dl(uint32_t nof_prb, bool ngap_is_1) {
  uint32_t ngap = srslte_ra_type2_ngap(nof_prb, ngap_is_1);
  if (ngap_is_1) {
    return 2 * (ngap < (nof_prb - ngap) ? ngap : nof_prb - ngap);
  } else {
    return ((uint32_t) nof_prb / ngap) * 2 * ngap;
  }
}

/* Table 7.1.7.2.1-1: Transport block size table on 36.213 */
int srslte_ra_tbs_from_idx(uint32_t tbs_idx, uint32_t n_prb) {
  if (tbs_idx < 27 && n_prb > 0 && n_prb <= SRSLTE_MAX_PRB) {
    return tbs_table[tbs_idx][n_prb - 1];
  } else {
    return SRSLTE_ERROR;
  }
}

/* Returns lowest nearest index of TBS value in table 7.1.7.2 on 36.213
 * or -1 if the TBS value is not within the valid TBS values
 */
int srslte_ra_tbs_to_table_idx(uint32_t tbs, uint32_t n_prb) {
  uint32_t idx;
  if (n_prb > 0 && n_prb <= SRSLTE_MAX_PRB) {
    return SRSLTE_ERROR;
  }
  if (tbs < tbs_table[0][n_prb]) {
    return SRSLTE_ERROR;
  }
  for (idx = 1; idx < 28; idx++) {
    if (tbs_table[idx - 1][n_prb] <= tbs && tbs_table[idx][n_prb] >= tbs) {
      return idx;
    }
  }
  return SRSLTE_ERROR;
}

void srslte_ra_pusch_fprint(FILE *f, srslte_ra_ul_dci_t *dci, uint32_t nof_prb) {
  fprintf(f, " - Resource Allocation Type 2 mode :\t%s\n",
      dci->type2_alloc.mode == SRSLTE_RA_TYPE2_LOC ? "Localized" : "Distributed");
  
  fprintf(f, "   + Frequency Hopping:\t\t\t");
  if (dci->freq_hop_fl == SRSLTE_RA_PUSCH_HOP_DISABLED) {
    fprintf(f, "No\n");
  } else {
    fprintf(f, "Yes\n");
  }
  fprintf(f, "   + Resource Indicator Value:\t\t%d\n", dci->type2_alloc.riv);
  if (dci->type2_alloc.mode == SRSLTE_RA_TYPE2_LOC) {
  fprintf(f, "   + VRB Assignment:\t\t\t%d VRB starting with VRB %d\n",
    dci->type2_alloc.L_crb, dci->type2_alloc.RB_start);
  } else {
  fprintf(f, "   + VRB Assignment:\t\t\t%d VRB starting with VRB %d\n",
    dci->type2_alloc.L_crb, dci->type2_alloc.RB_start);
  fprintf(f, "   + VRB gap selection:\t\t\tGap %d\n",
    dci->type2_alloc.n_gap == SRSLTE_RA_TYPE2_NG1 ? 1 : 2);
  fprintf(f, "   + VRB gap:\t\t\t\t%d\n",
    srslte_ra_type2_ngap(nof_prb, dci->type2_alloc.n_gap == SRSLTE_RA_TYPE2_NG1));

  }
  
  fprintf(f, " - Modulation and coding scheme index:\t%d\n", dci->mcs_idx);
  fprintf(f, " - New data indicator:\t\t\t%s\n", dci->ndi ? "Yes" : "No");
  fprintf(f, " - Redundancy version:\t\t\t%d\n", dci->rv_idx);
  fprintf(f, " - TPC command for PUCCH:\t\t--\n");    
}

void srslte_ra_ul_grant_fprint(FILE *f, srslte_ra_ul_grant_t *grant) {
  fprintf(f, " - Number of PRBs:\t\t\t%d\n", grant->L_prb);
  fprintf(f, " - Modulation type:\t\t\t%s\n", srslte_mod_string(grant->mcs.mod));
  fprintf(f, " - Transport block size:\t\t%d\n", grant->mcs.tbs);
}

char *ra_type_string(srslte_ra_type_t alloc_type) {
  switch (alloc_type) {
  case SRSLTE_RA_ALLOC_TYPE0:
    return "Type 0";
  case SRSLTE_RA_ALLOC_TYPE1:
    return "Type 1";
  case SRSLTE_RA_ALLOC_TYPE2:
    return "Type 2";
  default:
    return "N/A";
  }
}

void srslte_ra_pdsch_fprint(FILE *f, srslte_ra_dl_dci_t *dci, uint32_t nof_prb) {
  fprintf(f, " - Resource Allocation Type:\t\t%s\n",
      ra_type_string(dci->alloc_type));
  switch (dci->alloc_type) {
  case SRSLTE_RA_ALLOC_TYPE0:
    fprintf(f, "   + Resource Block Group Size:\t\t%d\n", srslte_ra_type0_P(nof_prb));
    fprintf(f, "   + RBG Bitmap:\t\t\t0x%x\n", dci->type0_alloc.rbg_bitmask);
    break;
  case SRSLTE_RA_ALLOC_TYPE1:
    fprintf(f, "   + Resource Block Group Size:\t\t%d\n", srslte_ra_type0_P(nof_prb));
    fprintf(f, "   + RBG Bitmap:\t\t\t0x%x\n", dci->type1_alloc.vrb_bitmask);
    fprintf(f, "   + RBG Subset:\t\t\t%d\n", dci->type1_alloc.rbg_subset);
    fprintf(f, "   + RBG Shift:\t\t\t\t%s\n",
        dci->type1_alloc.shift ? "Yes" : "No");
    break;
  case SRSLTE_RA_ALLOC_TYPE2:
    fprintf(f, "   + Type:\t\t\t\t%s\n",
        dci->type2_alloc.mode == SRSLTE_RA_TYPE2_LOC ? "Localized" : "Distributed");
    fprintf(f, "   + Resource Indicator Value:\t\t%d\n", dci->type2_alloc.riv);
    if (dci->type2_alloc.mode == SRSLTE_RA_TYPE2_LOC) {
      fprintf(f, "   + VRB Assignment:\t\t\t%d VRB starting with VRB %d\n",
          dci->type2_alloc.L_crb, dci->type2_alloc.RB_start);
    } else {
      fprintf(f, "   + VRB Assignment:\t\t\t%d VRB starting with VRB %d\n",
          dci->type2_alloc.L_crb, dci->type2_alloc.RB_start);
      fprintf(f, "   + VRB gap selection:\t\t\tGap %d\n",
          dci->type2_alloc.n_gap == SRSLTE_RA_TYPE2_NG1 ? 1 : 2);
      fprintf(f, "   + VRB gap:\t\t\t\t%d\n",
          srslte_ra_type2_ngap(nof_prb, dci->type2_alloc.n_gap == SRSLTE_RA_TYPE2_NG1));
    }
    break;
  }
  fprintf(f, " - Modulation and coding scheme index:\t%d\n", dci->mcs_idx);
  fprintf(f, " - HARQ process:\t\t\t%d\n", dci->harq_process);
  fprintf(f, " - New data indicator:\t\t\t%s\n", dci->ndi ? "Yes" : "No");
  fprintf(f, " - Redundancy version:\t\t\t%d\n", dci->rv_idx);
  fprintf(f, " - TPC command for PUCCH:\t\t--\n");
}

void srslte_ra_dl_grant_fprint(FILE *f, srslte_ra_dl_grant_t *grant) {
  srslte_ra_prb_fprint(f, grant);
  fprintf(f, " - Number of PRBs:\t\t\t%d\n", grant->nof_prb);
  fprintf(f, " - Modulation type:\t\t\t%s\n", srslte_mod_string(grant->mcs.mod));
  fprintf(f, " - Transport block size:\t\t%d\n", grant->mcs.tbs);
}

void srslte_ra_prb_fprint(FILE *f, srslte_ra_dl_grant_t *grant) {
  if (grant->nof_prb > 0) {
    for (int j=0;j<2;j++) {
      fprintf(f, " - PRB Bitmap Assignment %dst slot:\n", j);
      for (int i=0;i<SRSLTE_MAX_PRB;i++) {
        if (grant->prb_idx[j][i]) {
          fprintf(f, "%d, ", i);
        }
      }
      fprintf(f, "\n");      
    }
  }
  
}


