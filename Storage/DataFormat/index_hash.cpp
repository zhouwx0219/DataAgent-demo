#include "index_hash.h"
#include "table.h"

namespace storage {

	IndexHash::IndexHash() = default;

	IndexHash::~IndexHash() {
		std::unique_lock<std::shared_mutex> lk(latch_);
		map_.clear();
	}

	RC IndexHash::init() {
		std::unique_lock<std::shared_mutex> lk(latch_);
		map_.clear();
		map_.reserve(reserve_size_);
		return RCOK;
	}

	RC IndexHash::init(uint64_t size) {
		reserve_size_ = (size == 0 ? 1024 : size);
		std::unique_lock<std::shared_mutex> lk(latch_);
		map_.clear();
		map_.reserve(reserve_size_);
		return RCOK;
	}

	RC IndexHash::init(table_t * table) {
		this->table = table;
		return init(); // 用默认 reserve_size_
	}

	bool IndexHash::index_exist(idx_key_t key) {
		std::shared_lock<std::shared_mutex> lk(latch_);
		return map_.find(key) != map_.end();
	}

	RC IndexHash::index_insert(idx_key_t key, itemid_t * item, int part_id) {
		(void)part_id;
		if (item == nullptr) return Abort;

		std::unique_lock<std::shared_mutex> lk(latch_);
		map_[key] = item;
		return RCOK;
	}

	RC IndexHash::index_read(idx_key_t key, itemid_t * &item, int part_id) {
		(void)part_id;
		std::shared_lock<std::shared_mutex> lk(latch_);

		auto it = map_.find(key);
		if (it == map_.end()) {
			item = nullptr;
			return Abort;
		}

		item = it->second;
		return RCOK;
	}

	RC IndexHash::index_read(idx_key_t key, itemid_t * &item, int part_id, int thd_id) {
		(void)thd_id; // 简化版不使用线程ID
		return index_read(key, item, part_id);
	}

	RC IndexHash::index_remove(idx_key_t key) {
		std::unique_lock<std::shared_mutex> lk(latch_);
		auto it = map_.find(key);
		if (it == map_.end()) return Abort;
		map_.erase(it);
		return RCOK;
	}

} // namespace storage
