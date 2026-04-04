#include "../System/global.h"
#include "../System/helper.h"
#include "../System/txn.h"
#include "../DataFormat/row.h"
#include "occ.h"
#include "../System/manager.h"
#include "../System/mem_alloc.h"

namespace storage
{
	set_ent::set_ent() {
		set_size = 0;
		txn = NULL;
		rows = NULL;
		next = NULL;
	}

	void OptCC::init() {
		tnc = 0;
		his_len = 0;
		active_len = 0;
		active = NULL;
		history = NULL;
		lock_all = false;
		lock_txn_id = 0;
		pthread_mutex_init(&latch, NULL);
	}

	RC
	OptCC::validate(txn_man * txn) {
		RC rc = RCOK;
		for (int i = txn->row_cnt - 1; i > 0; i--) {
			for (int j = 0; j < i; j ++) {
				int tabcmp = strcmp(txn->accesses[j]->orig_row->get_table_name(),
				txn->accesses[j+1]->orig_row->get_table_name());
				if (tabcmp > 0 || (tabcmp == 0 && txn->accesses[j]->orig_row->get_primary_key() > txn->accesses[j+1]->orig_row->get_primary_key())) {
					Access * tmp = txn->accesses[j];
					txn->accesses[j] = txn->accesses[j+1];
					txn->accesses[j+1] = tmp;
				}
			}
		}

		// lock all rows in the readset and writeset.
		// Validate each access
		bool ok = true;
		txn->lock_cnt = 0;
		for (int i = 0; i < txn->row_cnt && ok; i++) {
			txn->lock_cnt ++;
			txn->accesses[i]->orig_row->manager->latch();
			ok = txn->accesses[i]->orig_row->manager->validate( txn->start_ts );
		}
		if (ok) {
			// Validation passed.
			// advance the global timestamp and get the end_ts
			txn->end_ts = glob_manager->get_ts( txn->get_thd_id() );
			// write to each row and update wts
			txn->cleanup(RCOK);
			rc = RCOK;
		} else {
			txn->cleanup(Abort);
			rc = Abort;
		}

		for (int i = 0; i < txn->lock_cnt; i++)
			txn->accesses[i]->orig_row->manager->release();

		return rc;
	}

	RC OptCC::get_rw_set(txn_man * txn, set_ent * &rset, set_ent *& wset) {
		wset = (set_ent*) mem_allocator.alloc(sizeof(set_ent), 0);
		rset = (set_ent*) mem_allocator.alloc(sizeof(set_ent), 0);
		wset->set_size = txn->wr_cnt;
		rset->set_size = txn->row_cnt - txn->wr_cnt;
		wset->rows = (row_t **) mem_allocator.alloc(sizeof(row_t *) * wset->set_size, 0);
		rset->rows = (row_t **) mem_allocator.alloc(sizeof(row_t *) * rset->set_size, 0);
		wset->txn = txn;
		rset->txn = txn;

		UInt32 n = 0, m = 0;
		for (int i = 0; i < txn->row_cnt; i++) {
			if (txn->accesses[i]->type == WR)
				wset->rows[n ++] = txn->accesses[i]->orig_row;
			else
				rset->rows[m ++] = txn->accesses[i]->orig_row;
		}

		assert(n == wset->set_size);
		assert(m == rset->set_size);
		return RCOK;
	}

	bool OptCC::test_valid(set_ent * set1, set_ent * set2) {
		for (UInt32 i = 0; i < set1->set_size; i++)
			for (UInt32 j = 0; j < set2->set_size; j++) {
				if (set1->rows[i] == set2->rows[j]) {
					return false;
				}
			}
		return true;
	}
}
