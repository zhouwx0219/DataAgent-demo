#include <sched.h>
#include "global.h"
#include "manager.h"
#include "thread.h"
#include "txn.h"
#include "wl.h"
#include "occ.h"
#include "mem_alloc.h"
#include "test.h"
namespace storage
{
	void thread_t::init(uint64_t thd_id, workload * workload) {
		_thd_id = thd_id;
		_wl = workload;
		srand48_r((_thd_id + 1) * get_sys_clock(), &buffer);
		_abort_buffer_size = ABORT_BUFFER_SIZE;
		_abort_buffer = (AbortBufferEntry *) _mm_malloc(sizeof(AbortBufferEntry) * _abort_buffer_size, 64);
		for (int i = 0; i < _abort_buffer_size; i++)
			_abort_buffer[i].query = NULL;
		_abort_buffer_empty_slots = _abort_buffer_size;
		_abort_buffer_enable = (g_params["abort_buffer_enable"] == "true");
	}

	uint64_t thread_t::get_thd_id() { return _thd_id; }
	uint64_t thread_t::get_host_cid() {	return _host_cid; }
	void thread_t::set_host_cid(uint64_t cid) { _host_cid = cid; }
	uint64_t thread_t::get_cur_cid() { return _cur_cid; }
	void thread_t::set_cur_cid(uint64_t cid) {_cur_cid = cid; }

	RC thread_t::run() {
	}

	ts_t
	thread_t::get_next_ts() {

	}

	RC thread_t::runTest(txn_man * txn)
	{

	}
}