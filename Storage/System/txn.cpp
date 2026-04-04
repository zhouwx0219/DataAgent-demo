//
// Created by zwx on 3/10/26.
//

#include "txn.h"
#include <cstring>
#include <cassert>

#include "DataFormat/row.h"
#include "thread.h"
#include "mem_alloc.h"
#include "concurrency_control/occ.h"
#include "DataFormat/catalog.h"
#include "DataFormat/index_hash.h"

namespace storage
{

std::hash<std::string> txn_man::hasher;

void txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
	this->h_thd = h_thd;
	this->h_wl = h_wl;
	pthread_mutex_init(&txn_lock, NULL);

	row_cnt = 0;
	wr_cnt = 0;
	lock_cnt = 0;
	insert_cnt = 0;

	accesses = (Access **) _mm_malloc(sizeof(Access *) * MAX_ROW_PER_TXN, 64);
	for (int i = 0; i < MAX_ROW_PER_TXN; i++)
		accesses[i] = NULL;

	num_accesses_alloc = 0;
	record_pos.clear();
}

void txn_man::set_txn_id(txnid_t txn_id) {
	this->txn_id = txn_id;
}

txnid_t txn_man::get_txn_id() {
	return this->txn_id;
}

workload * txn_man::get_wl() {
	return h_wl;
}

uint64_t txn_man::get_thd_id() {
	return h_thd->get_thd_id();
}

void txn_man::set_ts(ts_t timestamp) {
	this->timestamp = timestamp;
}

ts_t txn_man::get_ts() {
	return this->timestamp;
}

itemid_t * txn_man::index_read(INDEX * index, idx_key_t key, int part_id) {
	itemid_t * item = nullptr;
	index->index_read(key, item, part_id);
	return item;
}

void txn_man::index_read(INDEX * index, idx_key_t key, int part_id, itemid_t *& item) {
	index->index_read(key, item, part_id);
}

row_t * txn_man::get_row(row_t * row, access_t type) {
	RC rc = RCOK;
	uint64_t rid = row->get_primary_key();
	auto it = record_pos.find(rid);
	if (it != record_pos.end()) {
		Access *acc = accesses[it->second];
		if (acc->type == RD && type == WR) {
			acc->type = WR;
			wr_cnt++;
		}
		return acc->data;
	}
	if (accesses[row_cnt] == NULL) {
		Access * access = (Access *) _mm_malloc(sizeof(Access), 64);
		memset(access, 0, sizeof(Access));
		accesses[row_cnt] = access;
		num_accesses_alloc++;
	}

	Access *acc = accesses[row_cnt];
	rc = row->get_row(type, this, acc->data);
	if (rc == Abort) {
		return NULL;
	}

	acc->type = type;
	acc->orig_row = row;
	record_pos[rid] = row_cnt;
	row_cnt++;
	if (type == WR) wr_cnt++;

	return acc->data;
}

void txn_man::insert_row(row_t * row, table_t * table) {
	assert(insert_cnt < MAX_ROW_PER_TXN);
	insert_rows[insert_cnt++] = row;
}

RC txn_man::Read(const std::string &key, std::string &value_out, table_t * table) {
	//index search
	uint64_t primary_key = hash_key(key);
	INDEX* the_index = h_wl->indexes["Data_INDEX"];
	itemid_t *item = index_read(the_index, primary_key, 0);

	if (item == NULL) {
		return Abort;
	}

	//get row pointer and row data
	row_t *row = (row_t *)item->location;
	row_t *row_local = get_row(row, RD);
	if (!row_local) return Abort;

	//check key is correct or not
	char *read_key = row_local->get_value(0);
	if (!read_key || strcmp(read_key, key.c_str()) != 0) {
		return Abort;
	}

	//get row data
	char *read_val = row_local->get_value(1);
	value_out = read_val ? std::string(read_val) : "";
	return RCOK;
}

RC txn_man::Write(const std::string &key, const std::string &value, table_t * table) {
	uint64_t primary_key = hash_key(key);
	INDEX* the_index = h_wl->indexes["Data_INDEX"];
	table_t* the_table = h_wl->tables["Data_TABLE"];

	itemid_t *item = index_read(the_index, primary_key, 0);

	//key functions: the_table->get_new_row()
	// row->set_primary_key()
	// row->set_value()
	// the_index->index_insert()
	/// todo: record the write: update or insert
	assert(false);
	return RCOK;
}

RC txn_man::finish(RC rc) {
	if (rc == RCOK)
		rc = occ_man.validate(this);
	else
		cleanup(rc);
	return rc;
}

void txn_man::release() {
	for (int i = 0; i < num_accesses_alloc; i++)
		mem_allocator.free(accesses[i], 0);
	mem_allocator.free(accesses, 0);
}

void txn_man::cleanup(RC rc) {
	for (int rid = row_cnt - 1; rid >= 0; rid--) {
		row_t * orig_r = accesses[rid]->orig_row;
		access_t type = accesses[rid]->type;
		if (type == WR && rc == Abort)
			type = XP;
		orig_r->return_row(type, this, accesses[rid]->data);
		accesses[rid]->data = NULL;
	}

	if (rc == Abort) {
		for (UInt32 i = 0; i < insert_cnt; i++) {
			row_t * row = insert_rows[i];
			assert(g_part_alloc == false);
			row->free_row();
			mem_allocator.free(row, sizeof(row_t)); // 修复 sizeof(row)
		}
	}

	row_cnt = 0;
	wr_cnt = 0;
	insert_cnt = 0;
	record_pos.clear();
}

} // namespace storage
