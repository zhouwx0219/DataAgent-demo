#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
import time
import random
from dataclasses import dataclass, field
from typing import Optional, List

# =========================
# 配置
# =========================
HOST = "127.0.0.1"
PORT = 19090

THREADS = 10
TXN_PER_THREAD = 100
OPS_PER_TXN = 8
READ_RATIO = 0.5
ROLLBACK_RATIO = 0.2

KEY_SPACE = 20000

CONNECT_TIMEOUT = 300
SOCKET_TIMEOUT = 500
EOL = "\n"


CMD_START = "START"
CMD_GET = "GET"
CMD_PUT = "PUT"
CMD_COMMIT = "COMMIT"
CMD_ROLLBACK = "ROLLBACK"


# =========================
# 协议判断
# =========================
def is_ok(resp: str) -> bool:
    r = resp.strip().upper()
    # 根据你的服务返回格式可再扩展
    return (
            r.startswith("OK")
            or r.startswith("RCOK")
            or r.startswith("VALUE")
            or r.startswith("FOUND")
    )

def is_not_found(resp: str) -> bool:
    r = resp.strip().upper()
    return ("NOT_FOUND" in r) or ("NOKEY" in r) or ("MISS" in r)

def cmd_start() -> str:
    return f"{CMD_START}{EOL}"

def cmd_get(key: str) -> str:
    return f"{CMD_GET} {key}{EOL}"

def cmd_put(key: str, val: str) -> str:
    return f"{CMD_PUT} {key} {val}{EOL}"

def cmd_commit() -> str:
    return f"{CMD_COMMIT}{EOL}"

def cmd_rollback() -> str:
    return f"{CMD_ROLLBACK}{EOL}"


# =========================
# Session 客户端
# =========================
class SessionClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.sock = None
        self.buf = b""

    def _readline(self) -> str:
        while b"\n" not in self.buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("server closed connection")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return line.decode("utf-8", errors="replace")

    def connect(self):
        self.sock = socket.create_connection((self.host, self.port), timeout=CONNECT_TIMEOUT)
        self.sock.settimeout(SOCKET_TIMEOUT)

        # 关键：消费服务端欢迎消息，避免后续响应错位
        try:
            first = self._readline().strip()
            print(f"[DBG] <<< {first}")
            # 如果不是 welcome，也可按需放回/忽略；当前你的服务就是 welcome
        except socket.timeout:
            # 没有欢迎词也允许继续（兼容）
            pass

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    def request(self, cmd: str) -> str:
        if not self.sock:
            raise RuntimeError("session not connected")
        self.sock.sendall(cmd.encode("utf-8"))
        resp = self._readline()
        print(f"[DBG] >>> {cmd.strip()}")
        print(f"[DBG] <<< {resp.strip()}")
        return resp


@dataclass
class Stats:
    start_ok: int = 0
    start_fail: int = 0

    put_ok: int = 0
    put_fail: int = 0

    get_ok: int = 0
    get_not_found: int = 0
    get_fail: int = 0

    commit_ok: int = 0
    commit_fail: int = 0

    rollback_ok: int = 0
    rollback_fail: int = 0

    txn_success: int = 0
    txn_fail: int = 0

    conn_fail: int = 0
    exceptions: int = 0

    latency_ms: List[float] = field(default_factory=list)

    def merge(self, o: "Stats"):
        self.start_ok += o.start_ok
        self.start_fail += o.start_fail
        self.put_ok += o.put_ok
        self.put_fail += o.put_fail
        self.get_ok += o.get_ok
        self.get_not_found += o.get_not_found
        self.get_fail += o.get_fail
        self.commit_ok += o.commit_ok
        self.commit_fail += o.commit_fail
        self.rollback_ok += o.rollback_ok
        self.rollback_fail += o.rollback_fail
        self.txn_success += o.txn_success
        self.txn_fail += o.txn_fail
        self.conn_fail += o.conn_fail
        self.exceptions += o.exceptions
        self.latency_ms.extend(o.latency_ms)


def percentile(arr, p):
    if not arr:
        return 0.0
    s = sorted(arr)
    idx = int((len(s) - 1) * p)
    return s[idx]


def worker(tid: int, gstats: Stats, lock: threading.Lock):
    st = Stats()
    cli = SessionClient(HOST, PORT)
    rng = random.Random(20260405 + tid)

    try:
        cli.connect()
    except Exception:
        st.conn_fail += 1
        with lock:
            gstats.merge(st)
        return

    try:
        for tx in range(TXN_PER_THREAD):
            txn_ok = True
            t0 = time.perf_counter()

            try:
                # START
                resp = cli.request(cmd_start())
                if is_ok(resp):
                    st.start_ok += 1
                else:
                    st.start_fail += 1
                    txn_ok = False

                # TXN 内操作
                if txn_ok:
                    for i in range(OPS_PER_TXN):
                        key = f"k{rng.randint(1, KEY_SPACE)}"

                        if rng.random() < READ_RATIO:
                            resp = cli.request(cmd_get(key))
                            if is_ok(resp):
                                st.get_ok += 1
                            elif is_not_found(resp):
                                st.get_not_found += 1
                            else:
                                st.get_fail += 1
                                txn_ok = False
                                break
                        else:
                            val = f"t{tid}_tx{tx}_op{i}"
                            resp = cli.request(cmd_put(key, val))
                            if is_ok(resp):
                                st.put_ok += 1
                            else:
                                st.put_fail += 1
                                txn_ok = False
                                break

                # COMMIT / ROLLBACK
                if txn_ok and rng.random() >= ROLLBACK_RATIO:
                    resp = cli.request(cmd_commit())
                    if is_ok(resp):
                        st.commit_ok += 1
                        st.txn_success += 1
                    else:
                        st.commit_fail += 1
                        st.txn_fail += 1
                else:
                    # 如果中途失败或命中回滚比例，都尝试 rollback
                    resp = cli.request(cmd_rollback())
                    if is_ok(resp):
                        st.rollback_ok += 1
                    else:
                        st.rollback_fail += 1
                    # 回滚不算成功事务
                    st.txn_fail += 1

            except Exception:
                st.exceptions += 1
                st.txn_fail += 1
                # 尝试把会话状态拉回一致
                try:
                    cli.request(cmd_rollback())
                    st.rollback_ok += 1
                except Exception:
                    st.rollback_fail += 1

            st.latency_ms.append((time.perf_counter() - t0) * 1000.0)

    finally:
        cli.close()
        with lock:
            gstats.merge(st)


def main():
    print(f"[INFO] target={HOST}:{PORT}")
    print(f"[INFO] threads={THREADS}, txn/thread={TXN_PER_THREAD}, ops/txn={OPS_PER_TXN}")
    print(f"[INFO] read_ratio={READ_RATIO}, rollback_ratio={ROLLBACK_RATIO}")

    gstats = Stats()
    lock = threading.Lock()
    threads = []

    t0 = time.perf_counter()
    for tid in range(THREADS):
        th = threading.Thread(target=worker, args=(tid, gstats, lock), daemon=True)
        threads.append(th)
        th.start()

    for th in threads:
        th.join()
    t1 = time.perf_counter()

    total_txn = THREADS * TXN_PER_THREAD
    elapsed = t1 - t0
    tps = total_txn / elapsed if elapsed > 0 else 0.0

    p50 = percentile(gstats.latency_ms, 0.50)
    p95 = percentile(gstats.latency_ms, 0.95)
    p99 = percentile(gstats.latency_ms, 0.99)

    print("\n===== RESULT =====")
    print(f"total_txn         : {total_txn}")
    print(f"elapsed_sec       : {elapsed:.3f}")
    print(f"TPS               : {tps:.1f}")

    print(f"start_ok/fail     : {gstats.start_ok}/{gstats.start_fail}")
    print(f"put_ok/fail       : {gstats.put_ok}/{gstats.put_fail}")
    print(f"get_ok/not/fail   : {gstats.get_ok}/{gstats.get_not_found}/{gstats.get_fail}")
    print(f"commit_ok/fail    : {gstats.commit_ok}/{gstats.commit_fail}")
    print(f"rollback_ok/fail  : {gstats.rollback_ok}/{gstats.rollback_fail}")

    print(f"txn_success/fail  : {gstats.txn_success}/{gstats.txn_fail}")
    print(f"conn_fail         : {gstats.conn_fail}")
    print(f"exceptions        : {gstats.exceptions}")

    print(f"txn_latency_p50ms : {p50:.3f}")
    print(f"txn_latency_p95ms : {p95:.3f}")
    print(f"txn_latency_p99ms : {p99:.3f}")

if __name__ == "__main__":
    main()
