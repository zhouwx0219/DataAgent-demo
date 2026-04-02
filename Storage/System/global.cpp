//
// Created by zwx on 3/11/26.
//

#include "global.h"
#include "mem_alloc.h"
#include "manager.h"
#include "../concurrency_control/occ.h"

mem_alloc mem_allocator;
Manager * glob_manager;
Query_queue * query_queue;
OptCC occ_man;

bool volatile warmup_finish = false;
pthread_barrier_t warmup_bar;


std::atomic<uint64_t> global_ts_allocator;


UInt32 g_thread_cnt = THREAD_CNT;
std::map<string, string> g_params;

