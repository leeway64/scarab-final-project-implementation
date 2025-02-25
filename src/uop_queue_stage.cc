// The uop queue buffers ops fetched from the uop cache.

#include "uop_queue_stage.h"
#include "CircularQueue.h"

#include <deque>

extern "C" {
#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"
#include "bp/bp.h"
#include "op_pool.h"

#include "globals/assert.h"
#include "statistics.h"
#include "memory/memory.param.h"
#include "uop_cache.h"
// include external variabls
#include "icache_stage.h"
#include "dcache_stage.h"
#include "node_stage.h"
}

// Macros
#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_UOP_QUEUE_STAGE, ##args)
#define UOP_QUEUE_STAGE_LENGTH UOP_QUEUE_LENGTH
#define STAGE_MAX_OP_COUNT ISSUE_WIDTH  // The bandwidth of the next, consuming stage (map stage)
// TODO(peterbraun): Check if the ISSUE_WIDTH can be less than the uop cache issue bandwidth

// Uop Queue Variables
std::deque<Stage_Data*> q {};

// The circular queue needs to be initialized to the size of the instruction queue
CircularQueue<Stage_Data*> cq(UOP_QUEUE_STAGE_LENGTH);  // The circular queue containing the instructions

// Add current mode (SWQUE or queue)
int mpki_counter = 0;
int mpki = 0;
int flpi = 0;
bool circ_queue = true;
int low_priority_issues = 0;

std::deque<Stage_Data*> free_sds {};

// For uop queue fill stat
Counter last_recovery_cycle {};
Counter last_recovery_pw {};
std::size_t prev_q_size {};
bool uopq_off_path;

static inline void update_uop_queue_fill_time_stat(void);

/**************************************
Create a function that calcualtes priority
Based off how critical the path of the process 
is for assigning priority to SWQUE instruction
***************************************/

// Initialize either the queue or the circular queue here
void init_uop_queue_stage() {
  char tmp_name[MAX_STR_LENGTH + 1];
  for (uns ii = 0; ii < UOP_QUEUE_STAGE_LENGTH; ii++) {
    Stage_Data* sd = (Stage_Data*)calloc(1, sizeof(Stage_Data));
    snprintf(tmp_name, MAX_STR_LENGTH, "UOP QUEUE STAGE %d", ii);
    sd->name = (char*)strdup(tmp_name);
    sd->max_op_count = STAGE_MAX_OP_COUNT;
    sd->op_count = 0;
    sd->ops = (Op**)calloc(STAGE_MAX_OP_COUNT, sizeof(Op*));
    free_sds.push_back(sd);
  }

  //This handles statistics of the queue and operations, look at these in order to 
  //see how the flow of operation is going to switch between our own queue and the 
  //premade queue
  for (int cap_measured = 0; cap_measured < UOP_QUEUE_CAPACITY_MAX_MEASURED; cap_measured++) {
    char cycle_list_label[] = "Cycles to fill uop queue to size";
    char pw_list_label[] = "PWs to fill uop queue to size";
    char unique_pw_list_label[] = "Unique PWs to fill uop queue to size";
    init_list(&uop_queue_fill_time.time_for_size[cap_measured].cycles, cycle_list_label,
              sizeof(Counter), FALSE);
    init_list(&uop_queue_fill_time.time_for_size[cap_measured].pws, pw_list_label,
              sizeof(Counter), FALSE);
    init_list(&uop_queue_fill_time.time_for_size[cap_measured].unique_pws, unique_pw_list_label,
              sizeof(Counter), FALSE);
  }
}

void calculate_mpki(){
  if((icache_miss_count - dcache_miss_count) > 0){
    mpki_counter = icache_miss_count - (icache_miss_count - dcache_miss_count);
  }
  else{
    mpki_counter = dcache_miss_count - (dcache_miss_count - icache_miss_count);
  }
}

// Uncomment after circular queue has been implemented
void calculate_flpi(){
  if (issued_count > 0){
    // define low priority later when circulare queue is ready to go 
    flpi = low_priority_issues/issued_count;
  }
}

void count_priority_region(){
  // check if the queue has in its lower quartile
  if(cq.get_size() > cq.get_max_size() * .75){
    // count how many are in the lower issue queue
    low_priority_issues++;
  }
  else{
    // no priority issue
    low_priority_issues = 0;
  }
}
void switch_modes(){
  calculate_flpi();
  calculate_mpki();
  if(mpki_counter > MPKI_THRESHOLD || flpi > FLPI_THRESHOLD){
    circ_queue = true;
  }
  else{
    circ_queue = false;
  }
}
/***************************************/

// Get ops from the uop cache.
void update_uop_queue_stage(Stage_Data* src_sd) {
  // If the front of the queue was consumed, remove that stage.
  if (circ_queue) {
    if (cq.get_size() && cq.get_last()->op_count == 0) {
      free_sds.push_back(cq.get_last());
      cq.pop_last();
      ASSERT(0, !cq.get_size() || cq.get_last()->op_count > 0);
    }
  } else {
    cq.get_size();
    if (q.size() && q.front()->op_count == 0) {
      free_sds.push_back(q.front());
      q.pop_front();
      ASSERT(0, !q.size() || q.front()->op_count > 0);  // Only one stage is consumed per cycle
    }
  }

  update_uop_queue_fill_time_stat(); // gets updated the cycle after the size changes

  if (uopq_off_path) {
    STAT_EVENT(dec->proc_id, UOPQ_STAGE_OFF_PATH);
  }
  // If the queue cannot accomodate more ops, stall.
  if ((circ_queue && (uns)cq.get_size() >= UOP_QUEUE_STAGE_LENGTH) || (!circ_queue && q.size() >= UOP_QUEUE_STAGE_LENGTH)) {
    // Backend stalls may force fetch to stall.
    if (!uopq_off_path) {
      STAT_EVENT(dec->proc_id, UOPQ_STAGE_STALLED);
    }
    return;
  }
  else if (!uopq_off_path) {
    STAT_EVENT(dec->proc_id, UOPQ_STAGE_NOT_STALLED);
  }

  // Build a new sd and place new ops into the queue.
  Stage_Data* new_sd = free_sds.front();
  ASSERT(0, src_sd->op_count <= (int)STAGE_MAX_OP_COUNT);
  if (src_sd->op_count) {
    if (!uopq_off_path) {
      STAT_EVENT(dec->proc_id, UOPQ_STAGE_NOT_STARVED);
    }
    for (int i = 0; i < src_sd->max_op_count; i++) {
      Op* src_op = src_sd->ops[i];
      if (src_op) {
        ASSERT(src_op->proc_id, src_op->fetched_from_uop_cache);
        new_sd->ops[new_sd->op_count] = src_op;
        src_sd->ops[i] = NULL;
        new_sd->op_count++;
        src_sd->op_count--;
        decode_stage_process_op(src_op);
        DEBUG(0, "Fetching opnum=%llu\n", src_op->op_num);
        if (src_op->off_path) {
          uopq_off_path = true;
        }
      }
    }
  }
  else if (!uopq_off_path) {
    STAT_EVENT(dec->proc_id, UOPQ_STAGE_STARVED);
  }

  if (new_sd->op_count > 0) {
    free_sds.pop_front();
    if (circ_queue) {
      cq.push(new_sd);
    }
    else {
      q.push_back(new_sd);
    }
  }
}

void recover_uop_queue_stage(void) {
  uopq_off_path = false;
  if (circ_queue) {
    for (auto it = cq.begin(); it != cq.end();) {
      Stage_Data* sd = *it;
      sd->op_count = 0;
      for (uns op_index = 0; op_index < STAGE_MAX_OP_COUNT; op_index++) {
        Op* op = sd->ops[op_index];
        if (op && FLUSH_OP(op)) {
          ASSERT(op->proc_id, op->off_path);
          free_op(op);
          sd->ops[op_index] = NULL;
        } else if (op)
        {
          sd->op_count++;
        }
      }

      if (sd->op_count == 0) {
        free_sds.push_back(sd);
        it = cq.erase(it);
      } else {
        ++it;
      }
    }
  } else {
    for (std::deque<Stage_Data*>::iterator it = q.begin(); it != q.end();) {
      Stage_Data* sd = *it;
      sd->op_count = 0;
      for (uns op_idx = 0; op_idx < STAGE_MAX_OP_COUNT; op_idx++) {
        Op* op = sd->ops[op_idx];
        if (op && FLUSH_OP(op)) {
          ASSERT(op->proc_id, op->off_path);
          free_op(op);
          sd->ops[op_idx] = NULL;
        } else if (op) {
          sd->op_count++;
        }
      }

      if (sd->op_count == 0) {  // entire stage data was off-path
        free_sds.push_back(sd);
        it = q.erase(it);
      } else {
        ++it;
      }
    }
  }
  

  // TODO(peterbraun): This ignores effect of fetch barriers.
  last_recovery_cycle = cycle_count;
  last_recovery_pw = pw_count;
  prev_q_size = 0;  // This triggers the stat logging if the queue is not fully flushed
}

Stage_Data* uop_queue_stage_get_latest_sd(void) {
  if (circ_queue) {
    if (cq.get_size()) {
      return cq.get_last();
    }
  } else {
    if (q.size()) {
      return q.front();
    }    
  }

  ASSERT(0, free_sds.size() == UOP_QUEUE_STAGE_LENGTH);
  return free_sds.front();
};

int get_uop_queue_stage_length(void) {
  if (circ_queue) {
    return cq.get_size();
  } else {
    return q.size();
  }
}

// This is called each cycle. If size increased, log the time.
void update_uop_queue_fill_time_stat() {
  if (circ_queue) {
    if ((std::size_t)cq.get_size() > prev_q_size) {
      prev_q_size = cq.get_size();
      if (cq.get_size() <= UOP_QUEUE_CAPACITY_MAX_MEASURED) {
        Counter* new_cycle_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[cq.get_size() - 1].cycles));
        *new_cycle_entry = cycle_count - last_recovery_cycle;
        Counter* new_pw_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[cq.get_size() - 1].pws));
        *new_pw_entry = pw_count - last_recovery_pw;
        Counter* new_unique_pw_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[cq.get_size() - 1].unique_pws));
        *new_unique_pw_entry = unique_pws_since_recovery;
      }
    }
  } else {
    if (q.size() > prev_q_size) {
      prev_q_size = q.size();
      if (q.size() <= UOP_QUEUE_CAPACITY_MAX_MEASURED) {
        Counter* new_cycle_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[q.size()-1].cycles));
        *new_cycle_entry = cycle_count - last_recovery_cycle;
        Counter* new_pw_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[q.size()-1].pws));
        *new_pw_entry = pw_count - last_recovery_pw;
        Counter* new_unique_pw_entry = static_cast<Counter*>(sl_list_add_tail(&uop_queue_fill_time.time_for_size[q.size()-1].unique_pws));
        *new_unique_pw_entry = unique_pws_since_recovery;
      }
    }
  }
}
