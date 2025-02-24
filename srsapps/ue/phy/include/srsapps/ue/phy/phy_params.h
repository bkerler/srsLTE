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



#include "srslte/srslte.h"
#include "srsapps/common/params_db.h"

#ifndef PHYPARAMS_H
#define PHYPARAMS_H


namespace srslte {
namespace ue {

  class phy_params : public params_db
  {
  public: 

    phy_params() : params_db(NOF_PARAMS) { }
   ~phy_params() {}
    
    
    typedef enum {
      
      DL_FREQ = 0, 
      UL_FREQ, 

      CELLSEARCH_TIMEOUT_PSS_NFRAMES, 
      CELLSEARCH_TIMEOUT_MIB_NFRAMES, 
      CELLSEARCH_TIMEOUT_PSS_CORRELATION_THRESHOLD, // integer that will be divided by 10 

      PUSCH_BETA, 
      PUSCH_EN_64QAM,
      PUSCH_RS_CYCLIC_SHIFT,
      PUSCH_RS_GROUP_ASSIGNMENT,
      DMRS_GROUP_HOPPING_EN,
      DMRS_SEQUENCE_HOPPING_EN,
      
      PUSCH_HOPPING_N_SB,
      PUSCH_HOPPING_INTRA_SF,
      PUSCH_HOPPING_OFFSET,
      
      PUCCH_BETA, 
      PUCCH_DELTA_SHIFT,
      PUCCH_CYCLIC_SHIFT,
      PUCCH_N_RB_2,
      PUCCH_N_PUCCH_1_0,
      PUCCH_N_PUCCH_1_1,
      PUCCH_N_PUCCH_1_2,
      PUCCH_N_PUCCH_1_3,
      PUCCH_N_PUCCH_1,
      PUCCH_N_PUCCH_2,
      PUCCH_N_PUCCH_SR,

      SR_CONFIG_INDEX,
      
      SRS_BETA,
      SRS_UE_TXCOMB, 
      SRS_UE_NRRC,
      SRS_UE_DURATION,
      SRS_UE_CONFIGINDEX,
      SRS_UE_BW,
      SRS_UE_HOP,
      SRS_UE_CS,
      SRS_UE_CYCLICSHIFT,
      SRS_CS_BWCFG,
      SRS_CS_SFCFG,
      SRS_CS_ACKNACKSIMUL,
      SRS_IS_CONFIGURED,
      
      CQI_PERIODIC_PMI_IDX,
      CQI_PERIODIC_SIMULT_ACK,
      CQI_PERIODIC_FORMAT_SUBBAND,
      CQI_PERIODIC_FORMAT_SUBBAND_K,
      CQI_PERIODIC_CONFIGURED,
            
      UCI_I_OFFSET_ACK,
      UCI_I_OFFSET_RI,
      UCI_I_OFFSET_CQI,
      
      PRACH_CONFIG_INDEX,
      PRACH_ROOT_SEQ_IDX,
      PRACH_HIGH_SPEED_FLAG,
      PRACH_ZC_CONFIG,
      PRACH_FREQ_OFFSET,
      
      NOF_PARAMS,    
    } phy_param_t;
    
  };
}
}

#endif
