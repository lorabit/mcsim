/*
 * Copyright (c) 2010 The Hewlett-Packard Development Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jung Ho Ahn
 */

#ifndef PTS_MEMORYCONTROLLER_H
#define PTS_MEMORYCONTROLLER_H

#include "McSim.h"
#include <list>
#include <stack>
#include <queue>
#include <vector>
#include <set>


// some of the acronyms used here
// ------------------------------
//  pd : powerdown
//  vmd : virtual memory device

using namespace std;

namespace PinPthread
{
  enum mc_bank_action
  {
    mc_bank_activate,
    mc_bank_read,
    mc_bank_write,
    mc_bank_precharge,
    mc_bank_idle,
  };

  enum mc_scheduling_policy
  {
    mc_scheduling_open,
    mc_scheduling_closed,
    mc_scheduling_pred,
  };

  class MemoryController : public Component
  {
    public:
      class BankStatus
      {
        public:
          uint64_t action_time;
          uint64_t page_num;
          mc_bank_action action_type;
          uint64_t last_activate_time;
          list< pair<uint64_t, bool> > cached_pages;  // page number, dirty
          vector<uint32_t> bimodal_entry;             // 0 -- strongly open

          BankStatus(uint32_t num_entries) :action_time(0), page_num(0), action_type(mc_bank_idle),
              last_activate_time(0), cached_pages(), bimodal_entry(num_entries, 0)
          {
          }
      };

      MemoryController(component_type type_, uint32_t num_, McSim * mcsim_);
      ~MemoryController();

      void add_req_event(uint64_t, LocalQueueElement *, Component * from = NULL);
      void add_rep_event(uint64_t, LocalQueueElement *, Component * from = NULL);
      uint32_t process_event(uint64_t curr_time);

      //Directory * directory;  // uplink
      Component * directory;  // uplink
      vector<LocalQueueElement *> req_l;
      int32_t curr_batch_last;

      const uint32_t mc_to_dir_t;
      const uint32_t num_ranks_per_mc;
      const uint32_t num_banks_per_rank;

    private:
      // The unit of each t* value is "process_interval"
      // assume that RL = WL (in terms of DDRx)
      const uint32_t tRCD;
      const uint32_t tRR;
      uint32_t       tRP;
      const uint32_t tCL;           // CAS latency
      const uint32_t tBL;           // burst length
      const uint32_t tBBL;          // bank burst length
      const uint32_t tRAS;          // activate to precharge
      // send multiple column level commands
      const uint32_t tWRBUB;        // WR->RD bubble between any ranks
      const uint32_t tRWBUB;        // RD->WR bubble between any ranks
      const uint32_t tRRBUB;        // RD->RD bubble between two different ranks
      const uint32_t tWTR;          // WR->RD time in the same rank
      const uint32_t req_window_sz; // up to how many requests can be considered during scheduling
      uint32_t       interleave_xor_base_bit;

      uint32_t mc_to_dir_t_ab;
      uint32_t tRCD_ab;
      uint32_t tRAS_ab;
      uint32_t tRP_ab;
      uint32_t tCL_ab;
      bool     last_time_from_ab;
      uint32_t num_banks_with_agile_row;
      uint32_t reciprocal_of_agile_row_portion;
    public:
      const uint32_t rank_interleave_base_bit;
      const uint32_t bank_interleave_base_bit;
      const uint64_t page_sz_base_bit;   // byte addressing
      const uint32_t mc_interleave_base_bit;
      const uint32_t num_mcs;
    private:
      mc_scheduling_policy policy;
      bool           mini_rank;
      bool           par_bs;  // parallelism-aware batch-scheduling
      uint64_t      refresh_interval;
      uint64_t      curr_refresh_page;
      uint64_t      curr_refresh_bank;
      uint64_t      num_pages_per_bank;
      uint64_t      num_cached_pages_per_bank;
      bool          full_duplex;
      bool          is_fixed_latency;       // infinite BW
      bool          is_fixed_bw_n_latency;  // take care of BW as well
      uint32_t bimodal_entry;               // global predictor : 0 -- strongly open
      uint64_t * pred_history;              // size : num_hthreads
      uint32_t addr_offset_lsb;

      uint64_t num_read;
      uint64_t num_write;
      uint64_t num_activate;
      uint64_t num_precharge;
      uint64_t num_ab_read;
      uint64_t num_ab_write;
      uint64_t num_ab_activate;
      uint64_t num_write_to_read_switch;
      uint64_t num_refresh;  // a refresh command is applied to all VMD/BANK in a rank
      uint64_t num_pred_miss;
      uint64_t num_pred_hit;
      uint64_t num_global_pred_miss;
      uint64_t num_global_pred_hit;
      uint32_t num_pred_entries;

      uint32_t base0, base1, base2;
      uint32_t width0, width1, width2;

      uint64_t num_rw_interval;
      uint64_t num_conflict_interval;
      uint64_t num_pre_interval;

      uint64_t last_process_time;
      uint64_t packet_time_in_mc_acc;

      vector< vector<BankStatus> > bank_status;       // [rank][bank]
      vector< uint64_t > last_activate_time;          // [rank]
      vector< uint64_t > last_write_time;             // [rank]
      pair< uint32_t, uint64_t > last_read_time;      // <rank, tick>
      vector< uint64_t > last_read_time_rank;         // [rank]
      vector< bool >     is_last_time_write;          // [rank]
      map<uint64_t, mc_bank_action> dp_status;    // reuse (RD,WR,IDLE) BankStatus
      map<uint64_t, mc_bank_action> rd_dp_status; // reuse (RD,WR,IDLE) BankStatus
      map<uint64_t, mc_bank_action> wr_dp_status; // reuse (RD,WR,IDLE) BankStatus

    public:
      map<uint64_t, uint64_t> os_page_acc_dist;       // os page access distribution
      map<uint64_t, uint64_t> os_page_acc_dist_curr;  // os page access distribution
      bool     display_os_page_usage_summary;
      bool     display_os_page_usage;
      uint64_t num_reqs;

      void     update_acc_dist();

    private:
      uint64_t get_page_num(uint64_t addr);
      void show_state(uint64_t curr_time);

      bool     pre_processing(uint64_t curr_time);  // returns if the command was already sent or not.
      void     check_bank_status(LocalQueueElement * local_event);

      uint32_t num_hthreads;
      int32_t * num_req_from_a_th;

      inline uint32_t get_rank_num(uint64_t addr) { return ((addr >> rank_interleave_base_bit) ^ (addr >> interleave_xor_base_bit)) % num_ranks_per_mc; }
      inline uint32_t get_bank_num(uint64_t addr) {
        uint32_t num_banks_per_rank_curr = (addr >> 63 != 0 && reciprocal_of_agile_row_portion == 0 ? num_banks_with_agile_row : num_banks_per_rank); 
        uint32_t bank_num = ((addr >> bank_interleave_base_bit) ^ (addr >> interleave_xor_base_bit)) % num_banks_per_rank_curr;
        return bank_num;
      }
  };


}

#endif

