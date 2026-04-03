//
// Created by zwx on 4/2/26.
//

#ifndef KV_API_H
#define KV_API_H


#include <string>
#include <cstdint>

namespace server
{
    enum class Rc {
        OK = 0,
        NOT_FOUND,
        ABORT,
        ERROR
    };

    // 你现有 txn_man / txn context 的薄包装
    struct TxnHandle {
        void *impl {nullptr};   // 指向你的 txn_man*
        uint64_t id {0};
    };

    bool init_engine();   // main 启动时调用一次

    Rc begin_txn(TxnHandle &h);
    Rc get(TxnHandle &h, const std::string &key, std::string &value_out);
    Rc put(TxnHandle &h, const std::string &key, const std::string &value);
    Rc commit(TxnHandle &h);
    Rc abort_txn(TxnHandle &h);
}

#endif //KV_API_H
