//
// Created by zwx on 3/10/26.
//
#pragma once
#ifndef TXN_H
#define TXN_H

#include "global.h"
#include "helper.h"
namespace storage
{
	class workload;
	class thread_t;
	class row_t;
	class table_t;
	class base_query;
	class INDEX;


	// each thread has a txn_man.
	// a txn_man corresponds to a single transaction.

	class Access {
	public:
		access_t 	type;
		row_t * 	orig_row;
		row_t * 	data;
		row_t * 	orig_data;
		void cleanup();
	};

	class txn_man
	{
	public:
		virtual void init(thread_t * h_thd, workload * h_wl, uint64_t part_id);
		void release();
		thread_t * h_thd;
		workload * h_wl;
		myrand * mrand;
		uint64_t abort_cnt;

		virtual RC 		run_txn(base_query * m_query) = 0;
		uint64_t 		get_thd_id();
		workload * 		get_wl();
		void 			set_txn_id(txnid_t txn_id);
		txnid_t 		get_txn_id();

		void 			set_ts(ts_t timestamp);
		ts_t 			get_ts();

		pthread_mutex_t txn_lock;
		row_t * volatile cur_row;

		RC 				finish(RC rc);
		void 			cleanup(RC rc);

		// For OCC
		uint64_t 		start_ts;
		uint64_t 		end_ts;
		// following are public for OCC
		int 			row_cnt;
		int	 			wr_cnt;
		Access **		accesses;
		int 			num_accesses_alloc;

		itemid_t *		index_read(INDEX * index, idx_key_t key, int part_id);
		void 			index_read(INDEX * index, idx_key_t key, int part_id, itemid_t *& item);
		row_t * 		get_row(row_t * row, access_t type);

	protected:
		void 			insert_row(row_t * row, table_t * table);
	private:
		// insert rows
		uint64_t 		insert_cnt;
		row_t * 		insert_rows[MAX_ROW_PER_TXN];
		txnid_t 		txn_id;
		ts_t 			timestamp;

		bool _write_copy_ptr;

	};
}



#endif //TXN_H
