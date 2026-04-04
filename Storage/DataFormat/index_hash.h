#pragma once

#include "global.h"
#include "helper.h"
#include "index_base.h"
#include <unordered_map>
#include <shared_mutex>

namespace storage {

	class IndexHash : public index_base {
	public:
		IndexHash();
		~IndexHash();

		RC init() override;
		RC init(uint64_t size) override;
		RC init(table_t * table);

		bool index_exist(idx_key_t key) override;

		RC index_insert(idx_key_t key,
						itemid_t * item,
						int part_id = -1) override;

		RC index_read(idx_key_t key,
					  itemid_t * &item,
					  int part_id = -1) override;

		RC index_read(idx_key_t key,
					  itemid_t * &item,
					  int part_id,
					  int thd_id) override;


		RC index_remove(idx_key_t key) override;

	private:
		std::unordered_map<idx_key_t, itemid_t*> map_;
		mutable std::shared_mutex latch_;
		uint64_t reserve_size_ = 1024;
	};

} // namespace storage
