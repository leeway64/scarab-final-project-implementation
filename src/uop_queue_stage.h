// The uop queue is populated by uops from either the icache or the uop cache.

#ifndef __UOP_QUEUE_STAGE_H_
#define __UOP_QUEUE_STAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "op.h"
#include "globals/global_types.h"
#include "stage_data.h"
#include "decode_stage.h"
#include "uop_cache.h"

#define MPKI_THRESHOLD 0.04
#define FLPI_THRESHOLD 1 

// struct mode_switch_measures{
//     Counter llc_misses;
//     Counter retired_instructions;
//     double mpki;

//     Counter low_priority_issues;
//     Counter total_issues;
//     double flpi;
// }

void init_uop_queue_stage(void);
void update_uop_queue_stage(Stage_Data* src_sd);
void recover_uop_queue_stage(void);
Stage_Data* uop_queue_stage_get_latest_sd(void);
// Returns length of queue in terms of number of stages
int get_uop_queue_stage_length(void);
void stat_event_new_pw_accessed(Uop_Cache_Data* pw);

#ifdef __cplusplus
}
#endif

#endif