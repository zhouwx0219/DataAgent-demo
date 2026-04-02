//
// Created by zwx on 3/11/26.
//

#ifndef CONFIG_H
#define CONFIG_H


/***********************************************/
// Simulation + Hardware
/***********************************************/
#define THREAD_CNT					4
#define PART_CNT					1
#define VIRTUAL_PART_CNT			1
#define PAGE_SIZE					4096
#define CL_SIZE						64
// CPU_FREQ is used to get accurate timing info
#define CPU_FREQ 					2 	// in GHz/s

// print the transaction latency distribution
#define PRT_LAT_DISTR				false
#define STATS_ENABLE				true
#define TIME_ENABLE					true

#define MEM_ALLIGN					8

/***********************************************/
// Concurrency Control
/***********************************************/
#define ISOLATION_LEVEL 			SERIALIZABLE

// all transactions acquire tuples according to the primary key order.
#define KEY_ORDER					false
// transaction roll back changes after abort
#define ROLL_BACK					true
// per-row lock/ts management or central lock/ts management
#define CENTRAL_MAN					false
#define BUCKET_CNT					31
#define ABORT_PENALTY 				100000
#define ABORT_BUFFER_SIZE			10
#define ABORT_BUFFER_ENABLE			true
// [ INDEX ]
#define ENABLE_LATCH				false
#define CENTRAL_INDEX				false
#define CENTRAL_MANAGER 			false
#define INDEX_STRUCT				IDX_HASH
#define BTREE_ORDER 				16


// [OCC]
#define MAX_WRITE_SET				10
#define PER_ROW_VALID				true

/***********************************************/
// Logging
/***********************************************/
#define LOG_COMMAND					false
#define LOG_REDO					false
#define LOG_BATCH_TIME				10 // in ms


/***********************************************/
// Benchmark
/***********************************************/
// max number of rows touched per transaction
#define MAX_ROW_PER_TXN				64
#define QUERY_INTVL 				1UL
#define MAX_TXN_PER_PART 			100000
#define FIRST_PART_LOCAL 			true
#define MAX_TUPLE_SIZE				1024 // in bytes

// ==== [YCSB] ====
#define INIT_PARALLELISM			40
#define SYNTH_TABLE_SIZE 			(1024 * 1024 * 10)
#define ZIPF_THETA 					0.6
#define READ_PERC 					0.9
#define WRITE_PERC 					0.1
#define SCAN_PERC 					0
#define SCAN_LEN					20
#define PART_PER_TXN 				1
#define PERC_MULTI_PART				1
#define REQ_PER_QUERY				16
#define FIELD_PER_TUPLE				10

/***********************************************/
// Test cases
/***********************************************/
#define TEST_ALL					true
enum TestCases {
    READ_WRITE,
    CONFLICT
};
extern TestCases					g_test_case;

/***********************************************/
// DEBUG info
/***********************************************/
#define WL_VERB						true
#define IDX_VERB					false
#define VERB_ALLOC					true

#define DEBUG_LOCK					false
#define DEBUG_TIMESTAMP				false
#define DEBUG_SYNTH					false
#define DEBUG_ASSERT				false
#define DEBUG_CC					false //true



/***********************************************/
// Constant
/***********************************************/
// INDEX_STRUCT
#define IDX_HASH 					1
#define IDX_BTREE					2
// WORKLOAD
#define YCSB						1
#define TPCC						2
#define TEST						3
// Concurrency Control Algorithm
#define NO_WAIT						1
#define WAIT_DIE					2
#define DL_DETECT					3
#define TIMESTAMP					4
#define MVCC						5
#define HSTORE						6
#define OCC							7
#define TICTOC						8
#define SILO						9
#define VLL							10
#define HEKATON 					11
//Isolation Levels
#define SERIALIZABLE				1
#define SNAPSHOT					2
#define REPEATABLE_READ				3
// TIMESTAMP allocation method.
#define TS_MUTEX					1
#define TS_CAS						2
#define TS_HW						3
#define TS_CLOCK					4

#endif //CONFIG_H

