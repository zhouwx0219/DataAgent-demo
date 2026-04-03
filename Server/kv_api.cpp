//
// Created by zwx on 4/2/26.
//

#include "kv_api.h"

#include "kv_api.h"
#include <atomic>


#include "System/global.h"
#include "System/txn.h"
#include "DataFormat/table.h"
#include "DataFormat/index_hash.h"

namespace server {

static std::atomic<uint64_t> g_txn_id{1};

bool init_engine() {
    // TODO: 调你现有初始化逻辑（global/mem/table/index/workload等）
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