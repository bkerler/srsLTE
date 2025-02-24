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


#ifndef TTISYNC_H
#define TTISYNC_H


namespace srslte {
namespace ue {
  
  /* Interface used for PHY-MAC synchronization (producer-consumer model). 
   * The consumer waits while its counter is lower than the producer counter. 
   * The PHY is the consumer. The MAC is the producer. 
   */
class tti_sync
{
  public: 
    tti_sync(uint32_t modulus_) 
    {
      modulus = modulus_;
      increment = 1;
      init_counters(0);
    }
    virtual void     increase() = 0;
    virtual void     resync() = 0;
    virtual uint32_t wait() = 0; 
    virtual void     set_producer_cntr(uint32_t) = 0; 
    uint32_t         get_producer_cntr() { return producer_cntr; }
    uint32_t         get_consumer_cntr() { return consumer_cntr; }
    void             set_increment(uint32_t increment_) {
      increment = increment_; 
    }
  protected: 
    void increase_producer() { producer_cntr = (producer_cntr + increment)%modulus; }
    void increase_consumer() { consumer_cntr = (consumer_cntr + increment)%modulus; }
    bool wait_condition() { return producer_cntr == consumer_cntr; }
    void init_counters(uint32_t val) 
    { 
      consumer_cntr = val; 
      producer_cntr = val;      
    }
    uint32_t increment; 
    uint32_t modulus; 
    uint32_t producer_cntr;
    uint32_t consumer_cntr;
}; 
}
}

#endif
