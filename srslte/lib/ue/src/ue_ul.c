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

#include <complex.h>
#include <math.h>
#include <string.h>

#include "srslte/ue/ue_ul.h"

#define CURRENT_FFTSIZE   srslte_symbol_sz(q->cell.nof_prb)
#define CURRENT_SFLEN     SRSLTE_SF_LEN(CURRENT_FFTSIZE)

#define CURRENT_SLOTLEN_RE SRSLTE_SLOT_LEN_RE(q->cell.nof_prb, q->cell.cp)
#define CURRENT_SFLEN_RE SRSLTE_SF_LEN_RE(q->cell.nof_prb, q->cell.cp)


int srslte_ue_ul_init(srslte_ue_ul_t *q, 
                      srslte_cell_t cell) 
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  
  if (q                 != NULL &&
      srslte_cell_isvalid(&cell))   
  {
    ret = SRSLTE_ERROR;
    
    bzero(q, sizeof(srslte_ue_ul_t));
    
    q->cell = cell; 
    
    if (srslte_ofdm_tx_init(&q->fft, q->cell.cp, q->cell.nof_prb)) {
      fprintf(stderr, "Error initiating FFT\n");
      goto clean_exit;
    }
    srslte_ofdm_set_freq_shift(&q->fft, 0.5);
    srslte_ofdm_set_normalize(&q->fft, true);
    
    q->normalize_en = true; 

    if (srslte_cfo_init(&q->cfo, CURRENT_SFLEN)) {
      fprintf(stderr, "Error creating CFO object\n");
      goto clean_exit;
    }
    
    if (srslte_pusch_init(&q->pusch, q->cell)) {
      fprintf(stderr, "Error creating PUSCH object\n");
      goto clean_exit;
    }
    if (srslte_pucch_init(&q->pucch, q->cell)) {
      fprintf(stderr, "Error creating PUSCH object\n");
      goto clean_exit;
    }
    if (srslte_softbuffer_tx_init(&q->softbuffer, q->cell)) {
      fprintf(stderr, "Error initiating soft buffer\n");
      goto clean_exit;
    }
    if (srslte_refsignal_ul_init(&q->signals, cell)) {
      fprintf(stderr, "Error initiating srslte_refsignal_ul\n");
      goto clean_exit;
    }
    q->sf_symbols = srslte_vec_malloc(CURRENT_SFLEN_RE * sizeof(cf_t));
    if (!q->sf_symbols) {
      perror("malloc");
      goto clean_exit; 
    }
    q->refsignal = srslte_vec_malloc(2 * SRSLTE_NRE * q->cell.nof_prb * sizeof(cf_t));
    if (!q->refsignal) {
      perror("malloc");
      goto clean_exit; 
    }
    
    q->srs_signal = srslte_vec_malloc(SRSLTE_NRE * q->cell.nof_prb * sizeof(cf_t));
    if (!q->srs_signal) {
      perror("malloc");
      goto clean_exit; 
    }
    q->signals_pregenerated = false; 
    ret = SRSLTE_SUCCESS;
  } else {
    fprintf(stderr, "Invalid cell properties: Id=%d, Ports=%d, PRBs=%d\n",
            cell.id, cell.nof_ports, cell.nof_prb);      
  }

clean_exit: 
  if (ret == SRSLTE_ERROR) {
    srslte_ue_ul_free(q);
  }
  return ret;
}

void srslte_ue_ul_free(srslte_ue_ul_t *q) {
  if (q) {
    srslte_ofdm_rx_free(&q->fft);
    srslte_pusch_free(&q->pusch);
    srslte_pucch_free(&q->pucch);
    srslte_softbuffer_tx_free(&q->softbuffer);
    
    srslte_cfo_free(&q->cfo); 
    srslte_refsignal_ul_free(&q->signals);

    if (q->sf_symbols) {
      free(q->sf_symbols);
    }
    if (q->refsignal) {
      free(q->refsignal);
    }
    if (q->srs_signal) {
      free(q->srs_signal);
    }
    if (q->signals_pregenerated) {
      srslte_refsignal_dmrs_pusch_pregen_free(&q->signals, &q->pregen_drms);
      srslte_refsignal_srs_pregen_free(&q->signals, &q->pregen_srs);
    }
    bzero(q, sizeof(srslte_ue_ul_t));
  }
}

void srslte_ue_ul_set_cfo(srslte_ue_ul_t *q, float cur_cfo) {
  q->current_cfo = cur_cfo; 
}

void srslte_ue_ul_set_cfo_enable(srslte_ue_ul_t *q, bool enabled)
{
  q->cfo_en = enabled; 
}

void srslte_ue_ul_set_normalization(srslte_ue_ul_t *q, bool enabled)
{
  q->normalize_en = enabled;
}

/* Precalculate the PDSCH scramble sequences for a given RNTI. This function takes a while 
 * to execute, so shall be called once the final C-RNTI has been allocated for the session.
 * For the connection procedure, use srslte_pusch_encode_rnti() or srslte_pusch_decode_rnti() functions 
 */
void srslte_ue_ul_set_rnti(srslte_ue_ul_t *q, uint16_t rnti) {
  srslte_pusch_set_rnti(&q->pusch, rnti);
  srslte_pucch_set_crnti(&q->pucch, rnti);
  q->current_rnti = rnti; 
}

void srslte_ue_ul_reset(srslte_ue_ul_t *q) {
  srslte_softbuffer_tx_reset(&q->softbuffer);
}

int srslte_ue_ul_pregen_signals(srslte_ue_ul_t *q) {
  if (q->signals_pregenerated) {
    srslte_refsignal_dmrs_pusch_pregen_free(&q->signals, &q->pregen_drms);
    srslte_refsignal_srs_pregen_free(&q->signals, &q->pregen_srs);
  }
  if (srslte_refsignal_dmrs_pusch_pregen(&q->signals, &q->pregen_drms)) {
    return SRSLTE_ERROR;
  }
  if (srslte_refsignal_srs_pregen(&q->signals, &q->pregen_srs)) {
    return SRSLTE_ERROR;
  }
  q->signals_pregenerated = true; 
  return SRSLTE_SUCCESS;
}

void srslte_ue_ul_set_cfg(srslte_ue_ul_t *q, 
                          srslte_refsignal_dmrs_pusch_cfg_t *pusch_cfg, 
                          srslte_pucch_cfg_t *pucch_cfg, 
                          srslte_refsignal_srs_cfg_t *srs_cfg, 
                          srslte_pucch_sched_t *pucch_sched, 
                          bool group_hopping_en, 
                          bool sequence_hopping_en)
{
  srslte_refsignal_ul_set_cfg(&q->signals, pusch_cfg, pucch_cfg, srs_cfg, group_hopping_en, sequence_hopping_en);
  srslte_pucch_set_cfg(&q->pucch, pucch_cfg, group_hopping_en); 
  if (pucch_sched) {
    memcpy(&q->pucch_sched, pucch_sched, sizeof(srslte_pucch_sched_t));    
  }
}

int srslte_ue_ul_cfg_grant(srslte_ue_ul_t *q, srslte_dci_msg_t *dci_msg, 
                           srslte_pusch_hopping_cfg_t *hopping_cfg, 
                           srslte_refsignal_srs_cfg_t *srs_cfg, 
                           uint32_t tti, uint32_t cyclic_shift_for_dmrs, uint32_t rvidx) 
{
  return srslte_pusch_cfg(&q->pusch, &q->pusch_cfg, dci_msg, hopping_cfg, srs_cfg, tti, cyclic_shift_for_dmrs, rvidx);
}

/* Choose PUCCH format as in Sec 10.1 of 36.213 and generate PUCCH signal 
 */
int srslte_ue_ul_pucch_encode(srslte_ue_ul_t *q, srslte_uci_data_t uci_data, 
                              uint32_t tti, 
                              cf_t *output_signal)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  
  if (q             != NULL &&
      output_signal != NULL) 
  {
    
    uint32_t sf_idx = tti%10; 
    ret = SRSLTE_ERROR; 
    bzero(q->sf_symbols, sizeof(cf_t)*SRSLTE_SF_LEN_RE(q->cell.nof_prb, q->cell.cp));
    
    srslte_pucch_format_t format; 
    uint8_t pucch_bits[SRSLTE_PUCCH_MAX_BITS];
    uint8_t pucch2_bits[2];
    bzero(pucch_bits, SRSLTE_PUCCH_MAX_BITS*sizeof(uint8_t));
    bzero(pucch2_bits, 2*sizeof(uint8_t));
    
    // 1-bit ACK + optional SR
    if (uci_data.uci_ack_len == 1) {
      format = SRSLTE_PUCCH_FORMAT_1A;
      pucch_bits[0] = uci_data.uci_ack; 
    }
    // 2-bit ACK + optional SR
    else if (uci_data.uci_ack_len == 2) {
      format = SRSLTE_PUCCH_FORMAT_1B;    
      pucch_bits[0] = uci_data.uci_ack; 
      pucch_bits[1] = uci_data.uci_ack_2; 
    }
    // SR only 
    else if (uci_data.scheduling_request) {
      format = SRSLTE_PUCCH_FORMAT_1;    
    }
    // CQI and no ack
    else if (uci_data.uci_cqi_len == SRSLTE_PUCCH_MAX_BITS && uci_data.uci_ack_len == 0) {
      format = SRSLTE_PUCCH_FORMAT_2;    
      memcpy(pucch_bits, uci_data.uci_cqi, SRSLTE_PUCCH_MAX_BITS*sizeof(uint8_t));
    }
    // CQI + 1-bit ACK
    else if (uci_data.uci_cqi_len == SRSLTE_PUCCH_MAX_BITS && uci_data.uci_ack_len == 1) {
      format = SRSLTE_PUCCH_FORMAT_2A;    
      memcpy(pucch_bits, uci_data.uci_cqi, SRSLTE_PUCCH_MAX_BITS*sizeof(uint8_t));
      pucch2_bits[0] = uci_data.uci_ack; 
    }
    // CQI + 2-bit ACK 
    else if (uci_data.uci_cqi_len == 20 && uci_data.uci_ack_len == 2) {
      format = SRSLTE_PUCCH_FORMAT_2B;    
      memcpy(pucch_bits, uci_data.uci_cqi, SRSLTE_PUCCH_MAX_BITS*sizeof(uint8_t));
      pucch2_bits[0] = uci_data.uci_ack; 
      pucch2_bits[1] = uci_data.uci_ack_2; 
    }
    // CQI + 2-bit ACK + cyclic prefix 
    else if (uci_data.uci_cqi_len == 20 && uci_data.uci_ack_len == 1 && SRSLTE_CP_ISEXT(q->cell.cp)) {
      format = SRSLTE_PUCCH_FORMAT_2B;    
      memcpy(pucch_bits, uci_data.uci_cqi, SRSLTE_PUCCH_MAX_BITS*sizeof(uint8_t));
      pucch2_bits[0] = uci_data.uci_ack; 
      pucch2_bits[1] = uci_data.uci_ack_2; 
    } else {
      fprintf(stderr, "Unsupported combination of UCI parameters\n");
      return SRSLTE_ERROR; 
    }
    
    // Choose n_pucch 
    uint32_t n_pucch = 0; 
    if (uci_data.scheduling_request) {
      n_pucch = q->pucch_sched.n_pucch_sr; 
    } else if (format < SRSLTE_PUCCH_FORMAT_2) {
      if (q->pucch_sched.sps_enabled) {
        n_pucch = q->pucch_sched.n_pucch_1[q->pucch_sched.tpc_for_pucch%4];
      } else {
        n_pucch = q->pucch_sched.n_cce + q->pucch_sched.N_pucch_1; 
      }
    } else {
      n_pucch = q->pucch_sched.n_pucch_2; 
    }
    if (srslte_pucch_encode(&q->pucch, format, n_pucch, sf_idx, pucch_bits, q->sf_symbols)) {
      fprintf(stderr, "Error encoding TB\n");
      return ret; 
    }

    if (srslte_refsignal_dmrs_pucch_gen(&q->signals, format, n_pucch, sf_idx, pucch2_bits, q->refsignal)) 
    {
      fprintf(stderr, "Error generating PUSCH DRMS signals\n");
      return ret; 
    }
    srslte_refsignal_dmrs_pucch_put(&q->signals, format, n_pucch, q->refsignal, q->sf_symbols);                
    
    if (srslte_ue_ul_srs_tx_enabled(&q->signals.srs_cfg, tti) && q->pucch.shortened) {
      srslte_refsignal_srs_gen(&q->signals, tti%10, q->srs_signal);
      srslte_refsignal_srs_put(&q->signals, tti, q->srs_signal, q->sf_symbols);      
    }

    srslte_ofdm_tx_sf(&q->fft, q->sf_symbols, output_signal);
    
    if (q->cfo_en) {
      srslte_cfo_correct(&q->cfo, output_signal, output_signal, q->current_cfo / srslte_symbol_sz(q->cell.nof_prb));            
    }
    
    if (q->normalize_en) {
      float norm_factor = (float) 0.9*q->cell.nof_prb/5;
      srslte_vec_sc_prod_cfc(output_signal, norm_factor, output_signal, SRSLTE_SF_LEN_PRB(q->cell.nof_prb));
    }
    ret = SRSLTE_SUCCESS; 
  } 
  
  return ret; 
}

int srslte_ue_ul_pusch_encode(srslte_ue_ul_t *q, uint8_t *data, cf_t *output_signal) 
{
  srslte_uci_data_t uci_data;
  bzero(&uci_data, sizeof(srslte_uci_data_t));
  return srslte_ue_ul_pusch_uci_encode_rnti(q, data, uci_data, q->current_rnti, output_signal);    
}

int srslte_ue_ul_pusch_encode_rnti(srslte_ue_ul_t *q, uint8_t *data, uint16_t rnti, cf_t *output_signal)
{
  srslte_uci_data_t uci_data;
  bzero(&uci_data, sizeof(srslte_uci_data_t));
  return srslte_ue_ul_pusch_uci_encode_rnti(q, data, uci_data, rnti, output_signal);  
}

int srslte_ue_ul_pusch_uci_encode(srslte_ue_ul_t *q, uint8_t *data, srslte_uci_data_t uci_data, cf_t *output_signal)
{
  return srslte_ue_ul_pusch_uci_encode_rnti(q, data, uci_data, q->current_rnti, output_signal);
}

int srslte_ue_ul_pusch_uci_encode_rnti(srslte_ue_ul_t *q, 
                                       uint8_t *data, srslte_uci_data_t uci_data, 
                                       uint16_t rnti, 
                                       cf_t *output_signal)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  
  if (q             != NULL &&
      output_signal != NULL) 
  {
    
    if (q->pusch_cfg.grant.L_prb == 0)  {
      fprintf(stderr, "Invalid UL PRB allocation (L_prb=0)\n");
      return SRSLTE_ERROR; 
    }      
    
    return srslte_ue_ul_pusch_encode_rnti_softbuffer(q, data, uci_data, &q->softbuffer, rnti, output_signal); 
  }
  return ret; 
}

int srslte_ue_ul_srs_encode(srslte_ue_ul_t *q, uint32_t tti, cf_t *output_signal) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q && output_signal) {
    ret = SRSLTE_ERROR; 
    
    if (srslte_ue_ul_srs_tx_enabled(&q->signals.srs_cfg, tti)) {
      if (q->signals_pregenerated) {
        srslte_refsignal_srs_pregen_put(&q->signals, &q->pregen_srs, tti, q->sf_symbols);
      } else {
        srslte_refsignal_srs_gen(&q->signals, tti%10, q->srs_signal);
        srslte_refsignal_srs_put(&q->signals, tti, q->srs_signal, q->sf_symbols);      
      }
    }
    
    srslte_ofdm_tx_sf(&q->fft, q->sf_symbols, output_signal);
    
    if (q->cfo_en) {
      srslte_cfo_correct(&q->cfo, output_signal, output_signal, q->current_cfo / srslte_symbol_sz(q->cell.nof_prb));            
    }
    
    if (q->normalize_en) {
      float norm_factor = (float) q->cell.nof_prb/10/sqrtf(srslte_refsignal_srs_M_sc(&q->signals)/6);
      srslte_vec_sc_prod_cfc(output_signal, norm_factor, output_signal, SRSLTE_SF_LEN_PRB(q->cell.nof_prb));
    }
    
    ret = SRSLTE_SUCCESS; 
  } 
  return ret; 
}

int srslte_ue_ul_pusch_encode_rnti_softbuffer(srslte_ue_ul_t *q, 
                                              uint8_t *data, srslte_uci_data_t uci_data, 
                                              srslte_softbuffer_tx_t *softbuffer,
                                              uint16_t rnti, 
                                              cf_t *output_signal)
{
 
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  bzero(q->sf_symbols, sizeof(cf_t)*SRSLTE_SF_LEN_RE(q->cell.nof_prb, q->cell.cp));
  
  if (q             != NULL &&
      softbuffer    != NULL &&
      output_signal != NULL) 
  {
    if (srslte_pusch_uci_encode_rnti(&q->pusch, &q->pusch_cfg, softbuffer, data, uci_data, rnti, q->sf_symbols)) {
      fprintf(stderr, "Error encoding TB\n");
      return ret; 
    }

    if (q->signals_pregenerated) {
      srslte_refsignal_dmrs_pusch_pregen_put(&q->signals, &q->pregen_drms, 
                                             q->pusch_cfg.grant.L_prb, 
                                             q->pusch_cfg.sf_idx, 
                                             q->pusch_cfg.cyclic_shift_for_dmrs,
                                             q->pusch_cfg.grant.n_prb_tilde, 
                                             q->sf_symbols);
    } else {
      if (srslte_refsignal_dmrs_pusch_gen(&q->signals, q->pusch_cfg.grant.L_prb, 
                                          q->pusch_cfg.sf_idx,
                                          q->pusch_cfg.cyclic_shift_for_dmrs,
                                          q->refsignal)) 
      {
        fprintf(stderr, "Error generating PUSCH DRMS signals\n");
        return ret; 
      }
      srslte_refsignal_dmrs_pusch_put(&q->signals, q->refsignal, 
                                      q->pusch_cfg.grant.L_prb, 
                                      q->pusch_cfg.grant.n_prb_tilde, 
                                      q->sf_symbols);                      
    }

    if (srslte_ue_ul_srs_tx_enabled(&q->signals.srs_cfg, q->pusch_cfg.tti)) {
      if (q->signals_pregenerated) {
        srslte_refsignal_srs_pregen_put(&q->signals, &q->pregen_srs, q->pusch_cfg.tti, q->sf_symbols);
      } else {
        srslte_refsignal_srs_gen(&q->signals, q->pusch_cfg.sf_idx, q->srs_signal);
        srslte_refsignal_srs_put(&q->signals, q->pusch_cfg.tti, q->srs_signal, q->sf_symbols);              
      }
    }
    
    srslte_ofdm_tx_sf(&q->fft, q->sf_symbols, output_signal);
    
    if (q->cfo_en) {
      srslte_cfo_correct(&q->cfo, output_signal, output_signal, q->current_cfo / srslte_symbol_sz(q->cell.nof_prb));            
    }
    
    if (q->normalize_en) {
      float norm_factor = (float) q->cell.nof_prb/10/sqrtf(q->pusch_cfg.grant.L_prb);
      srslte_vec_sc_prod_cfc(output_signal, norm_factor, output_signal, SRSLTE_SF_LEN_PRB(q->cell.nof_prb));
    }
    
    ret = SRSLTE_SUCCESS; 
  } 
  
  return ret;   
}

/* Returns 1 if a SR needs to be sent at current_tti given I_sr, as defined in Section 10.1 of 36.213 */
int srslte_ue_ul_sr_send_tti(uint32_t I_sr, uint32_t current_tti) {
  uint32_t sr_periodicity; 
  uint32_t sr_N_offset;
  if (I_sr < 5) {
    sr_periodicity = 5;
    sr_N_offset    = I_sr; 
  } else if (I_sr < 15) {
    sr_periodicity = 10;
    sr_N_offset    = I_sr-5;     
  } else if (I_sr < 35) {
    sr_periodicity = 20;
    sr_N_offset    = I_sr-15; 
  } else if (I_sr < 75) {
    sr_periodicity = 40;
    sr_N_offset    = I_sr-35; 
  } else if (I_sr < 155) {
    sr_periodicity = 80;
    sr_N_offset    = I_sr-75; 
  } else if (I_sr < 157) {
    sr_periodicity = 2;
    sr_N_offset    = I_sr-155; 
  } else if (I_sr == 157) {
    sr_periodicity = 1;
    sr_N_offset    = I_sr-157; 
  } else {
    return SRSLTE_ERROR;
  }
  uint32_t sfn = current_tti/10;
  uint32_t subf = current_tti%10; 
  if ((10*sfn+subf-sr_N_offset)%sr_periodicity==0) {
    return 1; 
  } else {
    return SRSLTE_SUCCESS;
  }
}


bool srslte_ue_ul_srs_tx_enabled(srslte_refsignal_srs_cfg_t *srs_cfg, uint32_t tti) {
  if (srs_cfg->configured) {
    if (srslte_refsignal_srs_send_cs(srs_cfg->subframe_config, tti%10) == 1 && 
        srslte_refsignal_srs_send_ue(srs_cfg->I_srs, tti) == 1)
    {
      return true; 
    }
  }
  return false; 
}


