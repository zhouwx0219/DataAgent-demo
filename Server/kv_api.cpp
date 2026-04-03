//
// Created by zwx on 4/2/26.
//

#include "kv_api.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

#include "System/thread.h"
#include "System/global.h"
#include "System/mem_alloc.h"
#include "DataFormat/table.h"
#include "DataFormat/row.h"
#include "DataFormat/index_hash.h"
#include "test/wl.h"
#include "test/test.h"

namespace server {
using namespace std;

static std::atomic<uint64_t> g_txn_id{1};
storage::workload* g_wl = nullptr;
const std::string DATA_FILE = "./Storage/data.txt.txt";
const std::string SCHEMA_FILE = "./Storage/data_schema.txt";

// 简单的字符串哈希函数，将 string 转为 uint64_t 作为索引键
uint64_t hash_key(const std::string& key) {
    std::hash<std::string> hasher;
    return hasher(key);
}

bool init_engine() {

    std::cout << "[Engine] Initializing KV Engine..." << std::endl;
    g_wl = new storage::TestWorkload();
    g_wl->init();
    if (g_wl->init_schema(SCHEMA_FILE) != storage::RCOK) {
        std::cerr << "[Engine] Failed to load schema!" << std::endl;
        return false;
    }

    storage::table_t* the_table = g_wl->tables["Data_TABLE"];
    storage::INDEX* the_index = g_wl->indexes["Data_INDEX"];

    if (!the_table || !the_index) {
        std::cerr << "[Engine] Table or Index not found in schema!" << std::endl;
        return false;
    }

    // 3. 从文本文件加载数据到内存
    std::ifstream infile(DATA_FILE);
    if (!infile.is_open()) {
        std::cout << "[Engine] No existing data.txt file found. Starting fresh." << std::endl;
        return true;
    }

    std::string key_str, value_str;
    int loaded_count = 0;

    // 初始化一个虚拟的线程上下文，用于绑定事务
    // 假设主线程 ID 为 0
    storage::thread_t* h_thd = new storage::thread_t();
    h_thd->init(0, g_wl);

    while (infile >> key_str >> value_str) {
        uint64_t primary_key = hash_key(key_str);

        // 2. 开启事务 (Start Transaction)
        storage::txn_man* txn = nullptr;
        g_wl->get_txn_man(txn, h_thd); // 内部会调用 txn->init() 完成初始化

        storage::RC rc = storage::RCOK;

        // 3. 分配新行并设置数据
        storage::row_t* new_row = nullptr;
        uint64_t row_id;
        int part_id = 0; // 默认单分区

        rc = the_table->get_new_row(new_row, part_id, row_id);
        if (rc != storage::RCOK) {
            txn->finish(storage::Abort); // 分配失败，回滚事务
            continue;
        }

        // 写入列数据

        new_row->set_primary_key(primary_key);
        new_row->set_value(0, (void*) key_str.c_str());
        new_row->set_value(1, (void*) value_str.c_str());

        // 4. 封装为 itemid_t 并插入索引
        storage::itemid_t* m_item = (storage::itemid_t*) storage::mem_allocator.alloc(sizeof(storage::itemid_t), part_id);
        m_item->type = storage::DT_row;
        m_item->location = new_row;
        m_item->valid = true;

        rc = the_index->index_insert(primary_key, m_item, part_id);

        // 5. 提交事务 (Commit Transaction)
        if (rc == storage::RCOK) {
            // finish() 会触发 OCC 的 validate 阶段和最终的 commit
            rc = txn->finish(rc);
            if (rc == storage::RCOK || rc == storage::Commit) {
                loaded_count++;
            }
        } else {
            // 索引插入失败（如主键冲突），回滚事务
            txn->finish(storage::Abort);
        }

        // 注意：根据 DBx1000 的内存管理策略，txn 的内存可能由 mem_allocator 回收或复用
    }

    infile.close();
    std::cout << "[Engine] Successfully loaded " << loaded_count << " records from " << DATA_FILE << std::endl;
    // 返回 true 表示初始化成功
    return true;
}

Rc begin_txn(TxnHandle &h) {
    // TODO: 创建 txn_man，并赋给 h.impl
    // h.impl = create_txn_man(...);
    h.id = g_txn_id.fetch_add(1);
    if (!h.impl) {
        // 临时占位：如果你还没接真实 txn，可先用非空假值避免 session 层报错
        h.impl = reinterpret_cast<void*>(0x1);
    }
    return Rc::OK;
}

Rc get(TxnHandle &h, const std::string &key, std::string &value_out) {
    if (!h.impl) return Rc::ERROR;

    // TODO: 用 txn + index/table 读 key
    // rc_t rc = txn_read(h.impl, key, value_out);
    // if (rc == RCOK) return Rc::OK;
    // if (rc == NOT_FOUND) return Rc::NOT_FOUND;
    // if (rc == Abort) return Rc::ABORT;
    // return Rc::ERROR;

    // 占位演示
    (void)key;
    value_out = "mock_value";
    return Rc::OK;
}

Rc put(TxnHandle &h, const std::string &key, const std::string &value) {
    if (!h.impl) return Rc::ERROR;

    // TODO: 用 txn + index/table 写 key/value
    // rc_t rc = txn_write(h.impl, key, value);
    // if (rc == RCOK) return Rc::OK;
    // if (rc == Abort) return Rc::ABORT;
    // return Rc::ERROR;

    (void)key; (void)value;
    return Rc::OK;
}

Rc commit(TxnHandle &h) {
    if (!h.impl) return Rc::ERROR;

    // TODO: 调你现有 commit/validate 入口
    // rc_t rc = txn_commit(h.impl);
    // if (rc == RCOK) { h.impl = nullptr; return Rc::OK; }
    // if (rc == Abort) { h.impl = nullptr; return Rc::ABORT; }
    // h.impl = nullptr; return Rc::ERROR;

    h.impl = nullptr;
    return Rc::OK;
}

Rc abort_txn(TxnHandle &h) {
    if (!h.impl) return Rc::ERROR;

    // TODO: 调你现有 abort cleanup 入口
    // txn_abort(h.impl);

    h.impl = nullptr;
    return Rc::OK;
}

} // namespace