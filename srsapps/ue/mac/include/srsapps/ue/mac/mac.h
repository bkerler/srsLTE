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



#include <pthread.h>

#include "srsapps/common/log.h"
#include "srsapps/common/tti_sync.h"
#include "srsapps/common/tti_sync_cv.h"
#include "srsapps/ue/phy/phy.h"
#include "srsapps/ue/mac/mac_params.h"
#include "srsapps/ue/mac/dl_harq.h"
#include "srsapps/ue/mac/ul_harq.h"
#include "srsapps/ue/mac/dl_sps.h"
#include "srsapps/ue/mac/ul_sps.h"
#include "srsapps/common/timers.h"
#include "srsapps/ue/mac/mac_io.h"
#include "srsapps/ue/mac/proc_ra.h"
#include "srsapps/ue/mac/proc_sr.h"
#include "srsapps/ue/mac/proc_bsr.h"
#include "srsapps/ue/mac/proc_phr.h"
#include "srsapps/ue/mac/mux.h"
#include "srsapps/ue/mac/demux.h"
#include "srsapps/ue/mac/sdu_handler.h"
#include "srsapps/ue/mac/mac_pcap.h"
#include "srsapps/common/trace.h"


#ifndef UEMAC_H
#define UEMAC_H

namespace srslte {
namespace ue {
  
typedef _Complex float cf_t; 

class mac : public timer_callback
{
public:
  mac() : timers_db((uint32_t) NOF_MAC_TIMERS), tr_exec_total(1024*10), tr_exec_dl(1024*10), tr_exec_ul(1024*10) {started=false;  pcap = NULL; }
  bool init(phy *phy_h, tti_sync *ttisync, log *log_h);
  void stop();
  int  get_tti();
  void main_radio_loop(); // called after thread creation

  void add_sdu_handler(sdu_handler *handler); 
  
  bool send_sdu(uint32_t lcid, uint8_t *sdu_payload, uint32_t nbytes);
  bool send_ccch_sdu(uint8_t *sdu_payload, uint32_t nbytes);
  bool send_dtch0_sdu(uint8_t *sdu_payload, uint32_t nbytes);  // SRB0
  bool send_dcch0_sdu(uint8_t *sdu_payload, uint32_t nbytes);  // DRB0

  int  recv_sdu(uint32_t lcid, uint8_t *sdu_payload, uint32_t nbytes);
  int  recv_bcch_sdu(uint8_t *sdu_payload, uint32_t buffer_len_nbytes);
  int  recv_ccch_sdu(uint8_t *sdu_payload, uint32_t buffer_len_nbytes);
  int  recv_dtch0_sdu(uint8_t *sdu_payload, uint32_t buffer_len_nbytes); // SRB0
  int  recv_dcch0_sdu(uint8_t *sdu_payload, uint32_t buffer_len_nbytes); // DRB0

  void setup_lcid(uint32_t lcid, uint32_t lcg, uint32_t priority, int PBR_x_tti, uint32_t BSD);
  
  void set_param(mac_params::mac_param_t param, int64_t value); 
  int64_t get_param(mac_params::mac_param_t param);
  
  void reconfiguration(); 
  void reset(); 
  
  void start_trace();
  void write_trace(std::string filename); 
  
  void timer_expired(uint32_t timer_id); 
  void start_pcap(mac_pcap* pcap);
    
  enum {
    HARQ_RTT = 0, 
    TIME_ALIGNMENT,
    CONTENTION_TIMER,
    BSR_TIMER_PERIODIC,
    BSR_TIMER_RETX,
    NOF_MAC_TIMERS
  } mac_timers_t; 

  class tti_thread {
  public: 
    bool init(mac *parent, tti_sync_cv *ttysync);
    void run();
    void run_tti(uint32_t tti);
    void stop();
  private: 
    bool        started; 
    log         *log_h;
    mac         *parent; 
    tti_sync_cv *sync; 
    pthread_t   thread; 
  };

  static const int NOF_TTI_THREADS = 2; 
  
private:  
  
  
  // TTI processing threads
  tti_thread      tti_threads[NOF_TTI_THREADS]; 
  tti_sync_cv     tti_threads_sync[NOF_TTI_THREADS];
  pthread_mutex_t tti_threads_sync_tx[NOF_TTI_THREADS];
  bool            is_first_tx;

  // Interaction with PHY 
  tti_sync     *ttisync; 
  phy          *phy_h; 
  log          *log_h; 

  /* Logical channel (lch) IO */
  mac_io        mac_io_lch; 
  
  
  mac_params    params_db; 
  pthread_t     mac_thread; 
  static void*  mac_thread_fnc(void*);
  
  int           tti; 
  bool          started; 
  bool          is_synchronized; 
  uint16_t      last_temporal_crnti;
  
  /* Multiplexing/Demultiplexing Units */
  mux           mux_unit; 
  demux         demux_unit; 
  
  /* DL/UL HARQ */  
  dl_harq_entity dl_harq; 
  ul_harq_entity ul_harq; 
  
  /* DL/UL Semi-Persistent Sched */  
  dl_sps         dl_sps_assig;
  ul_sps         ul_sps_assig; 
  uint32_t       get_harq_sps_pid(uint32_t tti);
  
  /* MAC Uplink-related Procedures */
  ra_proc       ra_procedure;
  sr_proc       sr_procedure; 
  bsr_proc      bsr_procedure; 
  phr_proc      phr_procedure; 

  /* Other procedures */  
  void          process_dl_grants(uint32_t tti); 
  bool          process_ul_grants(uint32_t tti); 
  void          receive_pch(uint32_t tti);
  
  /* Functions for MAC Timers */
  timers        timers_db; 
  uint16_t      phy_rnti;
  void          setup_timers();
  void          timeAlignmentTimerExpire();

  // pointer to MAC PCAP object
  mac_pcap* pcap;
  
  // Variables for Execution time Trace
  trace<uint32_t> tr_exec_total;
  trace<uint32_t> tr_exec_dl;
  trace<uint32_t> tr_exec_ul;
  struct timeval tr_time_total[3];
  struct timeval tr_time_ul[3];
  struct timeval tr_time_dl[3];
  bool tr_enabled;   
  bool is_first_of_burst;
  void tr_log_start(uint32_t tti);
  void tr_log_end(uint32_t tti);    
  void tr_log_dl(uint32_t tti);
  void tr_log_ul(uint32_t tti);    
  void pregen_phy(uint16_t phy_rnti);

};

} 
}
#endif
