//
// Created by zwx on 4/2/26.
//

#include "kv_api.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring> // 引入 strncpy, strcmp

#include "manager.h"
#include "occ.h"
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
const std::string DATA_FILE = "../Storage/test/data.txt";
const std::string SCHEMA_FILE = "../Storage/test/data_schema.txt";

// 简单的字符串哈希函数，将 string 转为 uint64_t 作为索引键
uint64_t hash_key(const std::string& key) {
    std::hash<std::string> hasher;
    return hasher(key);
}

// 封装事务上下文，方便在 TxnHandle.impl 中透传
struct TxnContext {
    storage::txn_man* txn;
    storage::thread_t* thd;
};

bool init_engine() {
    std::cout << "[Engine] Initializing KV Engine..." << std::endl;
    g_wl = new storage::TestWorkload();
    g_wl->init();
    storage::glob_manager = (storage::Manager *) _mm_malloc(sizeof(storage::Manager), 64);
    storage::glob_manager->init();
    storage::mem_allocator.init(storage::g_part_cnt, MEM_SIZE / storage::g_part_cnt); ;
    storage::occ_man.init();

    storage::table_t* the_table = g_wl->tables["Data_TABLE"];
    storage::INDEX* the_index = g_wl->indexes["Data_INDEX"];

    if (!the_table || !the_index) {
        std::cerr << "[Engine] Table or Index not found in schema!" << std::endl;
        return false;
    }

    std::ifstream infile(DATA_FILE);
    if (!infile.is_open()) {
        std::cout << "[Engine] No existing data file found. Starting fresh." << std::endl;
        return true;
    }

    std::string key_str, value_str;
    int loaded_count = 0;

    storage::thread_t* h_thd = new storage::thread_t();
    h_thd->init(0, g_wl);

    while (infile >> key_str >> value_str) {
        uint64_t primary_key = hash_key(key_str);

        storage::txn_man* txn = nullptr;
        g_wl->get_txn_man(txn, h_thd);

        storage::RC rc = storage::RCOK;
        storage::row_t* new_row = nullptr;
        uint64_t row_id;
        int part_id = 0;

        rc = the_table->get_new_row(new_row, part_id, row_id);
        if (rc != storage::RCOK) {
            txn->finish(storage::Abort);
            continue;
        }

        char key_buf[128] = {0};
        char val_buf[1024] = {0};
        strncpy(key_buf, key_str.c_str(), 127);
        strncpy(val_buf, value_str.c_str(), 1023);

        new_row->set_primary_key(primary_key);
        new_row->set_value(0, key_buf);
        new_row->set_value(1, val_buf);

        storage::itemid_t* m_item = (storage::itemid_t*) storage::mem_allocator.alloc(sizeof(storage::itemid_t), part_id);
        m_item->type = storage::DT_row;
        m_item->location = new_row;
        m_item->valid = true;

        rc = the_index->index_insert(primary_key, m_item, part_id);

        if (rc == storage::RCOK) {
            rc = txn->finish(rc);
            if (rc == storage::RCOK || rc == storage::Commit) {
                loaded_count++;
            }
        } else {
            txn->finish(storage::Abort);
        }
    }

    infile.close();
    std::cout << "[Engine] Successfully loaded " << loaded_count << " records." << std::endl;
    return true;
}

Rc begin_txn(TxnHandle &h) {
    TxnContext* ctx = new TxnContext();
    ctx->thd = new storage::thread_t();
    ctx->thd->init(0, g_wl);

    g_wl->get_txn_man(ctx->txn, ctx->thd);
    ctx->txn->start_ts = storage::glob_manager->get_ts(ctx->txn->get_thd_id());

    h.id = g_txn_id.fetch_add(1);
    h.impl = ctx;
    return Rc::OK;
}

Rc get(TxnHandle &h, const std::string &key, std::string &value_out) {
    if (!h.impl) return Rc::ERROR;
    TxnContext* ctx = static_cast<TxnContext*>(h.impl);
    if (ctx->txn->Read(key, value_out) == storage::RCOK)
        return Rc::OK;
    else
        return Rc::NOT_FOUND;

}

Rc put(TxnHandle &h, const std::string &key, const std::string &value) {
    if (!h.impl) return Rc::ERROR;
    TxnContext* ctx = static_cast<TxnContext*>(h.impl);
    if ( ctx->txn->Write(key, value) == storage::RCOK)
        return Rc::OK;
    else
        return Rc::ERROR;

}

Rc commit(TxnHandle &h) {
    if (!h.impl) return Rc::ERROR;
    TxnContext* ctx = static_cast<TxnContext*>(h.impl);

    storage::RC rc = ctx->txn->finish(storage::RCOK);

    delete ctx->thd;
    delete ctx;
    h.impl = nullptr;

    if (rc == storage::RCOK || rc == storage::Commit) {
        return Rc::OK;
    }
    return Rc::ABORT;
}

Rc abort_txn(TxnHandle &h) {
    if (!h.impl) return Rc::ERROR;
    TxnContext* ctx = static_cast<TxnContext*>(h.impl);

    ctx->txn->finish(storage::Abort);

    delete ctx->thd;
    delete ctx;
    h.impl = nullptr;

    return Rc::OK;
}

} // namespace server
