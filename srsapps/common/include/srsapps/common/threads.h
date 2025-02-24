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
#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif
  bool threads_new_rt(pthread_t *thread, void *(*start_routine) (void*), void *arg);
  bool threads_new_rt_prio(pthread_t *thread, void *(*start_routine) (void*), void *arg, uint32_t prio_offset);
  bool threads_new_rt_cpu(pthread_t *thread, void *(*start_routine) (void*), void *arg, int cpu, uint32_t prio_offset);
  void threads_print_self();

#ifdef __cplusplus
}
#endif

