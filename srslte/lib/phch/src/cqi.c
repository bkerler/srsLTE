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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "srslte/phch/cqi.h"
#include "srslte/common/phy_common.h"
#include "srslte/utils/bit.h"
#include "srslte/utils/vector.h"
#include "srslte/utils/debug.h"


int srslte_cqi_hl_subband_pack(srslte_cqi_hl_subband_t *msg, uint32_t N, uint8_t *buff, uint32_t buff_len) 
{
  uint8_t *body_ptr = buff; 
  srslte_bit_pack(msg->wideband_cqi, &body_ptr, 4);
  srslte_bit_pack(msg->subband_diff_cqi, &body_ptr, 2*N);
  
  return 4+2*N;
}

int srslte_cqi_ue_subband_pack(srslte_cqi_ue_subband_t *msg, uint32_t L, uint8_t *buff, uint32_t buff_len)
{
  uint8_t *body_ptr = buff; 
  srslte_bit_pack(msg->wideband_cqi, &body_ptr, 4);
  srslte_bit_pack(msg->subband_diff_cqi, &body_ptr, 2);  
  srslte_bit_pack(msg->subband_diff_cqi, &body_ptr, L);  
  
  return 4+2+L;
}

int srslte_cqi_format2_wideband_pack(srslte_cqi_format2_wideband_t *msg, uint8_t *buff, uint32_t buff_len) 
{
  uint8_t *body_ptr = buff; 
  srslte_bit_pack(msg->wideband_cqi, &body_ptr, 4);  
  return 4;  
}

int srslte_cqi_format2_subband_pack(srslte_cqi_format2_subband_t *msg, uint8_t *buff, uint32_t buff_len) 
{
  uint8_t *body_ptr = buff; 
  srslte_bit_pack(msg->subband_cqi, &body_ptr, 4);  
  srslte_bit_pack(msg->subband_label, &body_ptr, 1);  
  return 4+1;    
}

bool srslte_cqi_send(uint32_t I_cqi_pmi, uint32_t tti) {
  
  uint32_t N_p = 0;
  uint32_t N_offset = 0;
  
  if (I_cqi_pmi <= 1) {
    N_p = 2; 
    N_offset = I_cqi_pmi; 
  } else if (I_cqi_pmi <= 6) {
    N_p = 5; 
    N_offset = I_cqi_pmi - 2;     
  } else if (I_cqi_pmi <= 16) {
    N_p = 10; 
    N_offset = I_cqi_pmi - 7;     
  } else if (I_cqi_pmi <= 36) {
    N_p = 20; 
    N_offset = I_cqi_pmi - 17;     
  } else if (I_cqi_pmi <= 76) {
    N_p = 40; 
    N_offset = I_cqi_pmi - 37;     
  } else if (I_cqi_pmi <= 156) {
    N_p = 80; 
    N_offset = I_cqi_pmi - 77;     
  } else if (I_cqi_pmi <= 316) {
    N_p = 160; 
    N_offset = I_cqi_pmi - 157;   
  } else if (I_cqi_pmi == 317) {
    return false; 
  } else if (I_cqi_pmi <= 349) {
    N_p = 32; 
    N_offset = I_cqi_pmi - 318;     
  } else if (I_cqi_pmi <= 413) {
    N_p = 64; 
    N_offset = I_cqi_pmi - 350;     
  } else if (I_cqi_pmi <= 541) {
    N_p = 128; 
    N_offset = I_cqi_pmi - 414;     
  } else if (I_cqi_pmi <= 1023) {
    return false; 
  }
  
  if ((tti-N_offset)%N_p == 0) {
    return true; 
  } else {
    return false; 
  }
}

