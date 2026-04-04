//
// Created by zwx on 4/3/26.
//

#include "session.h"
#include "server.h"
#include "kv_api.h"

#include <unistd.h>
#include <sstream>
#include <vector>
#include <algorithm>

namespace
{

    static std::vector<std::string> split_ws(const std::string &s) {
        std::istringstream iss(s);
        std::vector<std::string> v;
        std::string t;
        while (iss >> t) v.push_back(t);
        return v;
    }

    static std::string upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::toupper(c); });
        return s;
    }

}

namespace server
{
    void handle_client(int client_fd) {
        TxnHandle tx{};
        bool in_txn = false;

        send_all(client_fd, "WELCOME DataAgentDB\n");

        while (true) {
            bool ok = false;
            std::string line = read_line(client_fd, ok);
            if (!ok) {
                // 断连兜底
                if (in_txn) {
                    abort_txn(tx);
                    in_txn = false;
                }
                break;
            }

            auto parts = split_ws(line);
            if (parts.empty()) {
                send_all(client_fd, "ERR EMPTY_CMD\n");
                continue;
            }

            std::string cmd = upper(parts[0]);

            if (cmd == "START") {
                if (in_txn) {
                    send_all(client_fd, "ERR TXN_ALREADY_STARTED\n");
                    continue;
                }
                auto rc = begin_txn(tx);
                if (rc == Rc::OK) {
                    in_txn = true;
                    send_all(client_fd, "OK TXN " + std::to_string(tx.id) + "\n");
                } else {
                    send_all(client_fd, "ERR START_FAIL\n");
                }
            } else if (cmd == "GET") {
                if (!in_txn) {
                    send_all(client_fd, "ERR NO_ACTIVE_TXN\n");
                    continue;
                }
                if (parts.size() != 2) {
                    send_all(client_fd, "ERR USAGE GET <key>\n");
                    continue;
                }
                std::string val;
                auto rc = get(tx, parts[1], val);
                if (rc == Rc::OK) send_all(client_fd, "OK VALUE " + val + "\n");
                else if (rc == Rc::NOT_FOUND) send_all(client_fd, "ERR NOT_FOUND\n");
                else if (rc == Rc::ABORT) { in_txn = false; send_all(client_fd, "ERR ABORT\n"); }
                else send_all(client_fd, "ERR GET_FAIL\n");
            } else if (cmd == "PUT") {
                if (!in_txn) {
                    send_all(client_fd, "ERR NO_ACTIVE_TXN\n");
                    continue;
                }
                if (parts.size() < 3) {
                    send_all(client_fd, "ERR USAGE PUT <key> <value>\n");
                    continue;
                }
                auto pos1 = line.find(' ');
                auto pos2 = (pos1 == std::string::npos) ? std::string::npos : line.find(' ', pos1 + 1);
                std::string key = parts[1];
                std::string value = (pos2 == std::string::npos) ? "" : line.substr(pos2 + 1);

                auto rc = put(tx, key, value);
                if (rc == Rc::OK) send_all(client_fd, "OK\n");
                else if (rc == Rc::ABORT) { in_txn = false; send_all(client_fd, "ERR ABORT\n"); }
                else send_all(client_fd, "ERR PUT_FAIL\n");
            } else if (cmd == "COMMIT") {
                if (!in_txn) {
                    send_all(client_fd, "ERR NO_ACTIVE_TXN\n");
                    continue;
                }
                auto rc = commit(tx);
                in_txn = false;
                if (rc == Rc::OK) send_all(client_fd, "OK COMMIT\n");
                else if (rc == Rc::ABORT) send_all(client_fd, "ERR ABORT\n");
                else send_all(client_fd, "ERR COMMIT_FAIL\n");
            } else if (cmd == "ABORT") {
                if (!in_txn) {
                    send_all(client_fd, "ERR NO_ACTIVE_TXN\n");
                    continue;
                }
                (void)abort_txn(tx);
                in_txn = false;
                send_all(client_fd, "OK ABORT\n");
            } else if (cmd == "END" || cmd == "QUIT" || cmd == "EXIT") {
                if (in_txn) {
                    abort_txn(tx);
                    in_txn = false;
                }
                send_all(client_fd, "BYE\n");
                break;
            } else {
                send_all(client_fd, "ERR BAD_CMD\n");
            }
        }
    }
}