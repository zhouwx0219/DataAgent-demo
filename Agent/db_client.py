# Agent/db_client.py
from __future__ import annotations
from typing import Dict, Optional, Any
import uuid
import copy


class TxContext:
    def __init__(self, tx_id: str, snapshot: Dict[str, str]):
        self.tx_id = tx_id
        self.snapshot = snapshot       # 事务开始时快照（可用于回滚）
        self.writes: Dict[str, str] = {}  # 暂存写集
        self.active = True


class MiniDBClient:
    def __init__(self):
        self._store: Dict[str, str] = {}
        self._tx_map: Dict[str, TxContext] = {}

    # ---------- 非事务只读 ----------
    def kv_get(self, key: str) -> Optional[str]:
        return self._store.get(key)

    # ---------- 事务接口 ----------
    def start(self) -> str:
        tx_id = str(uuid.uuid4())
        snapshot = copy.deepcopy(self._store)
        self._tx_map[tx_id] = TxContext(tx_id, snapshot)
        return tx_id

    def _require_tx(self, tx_id: str) -> TxContext:
        tx = self._tx_map.get(tx_id)
        if tx is None or not tx.active:
            raise RuntimeError(f"invalid or inactive tx_id: {tx_id}")
        return tx

    def get(self, tx_id: str, key: str) -> Optional[str]:
        tx = self._require_tx(tx_id)
        if key in tx.writes:
            return tx.writes[key]
        return self._store.get(key)

    def put(self, tx_id: str, key: str, value: str) -> None:
        tx = self._require_tx(tx_id)
        tx.writes[key] = value

    def commit(self, tx_id: str) -> None:
        tx = self._require_tx(tx_id)
        for k, v in tx.writes.items():
            self._store[k] = v
        tx.active = False
        del self._tx_map[tx_id]

    def rollback(self, tx_id: str) -> None:
        tx = self._require_tx(tx_id)
        # 内存实现里未提交写集本就未生效，直接丢弃即可
        tx.active = False
        del self._tx_map[tx_id]

    # ---------- 调试 ----------
    def dump_all(self) -> Dict[str, Any]:
        return {"ok": True, "data": dict(self._store), "size": len(self._store)}


# 全局单例
mini_db = MiniDBClient()
