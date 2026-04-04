# Agent/tools/kv_tx_tools.py
from typing import Dict, Any
from db_client import mini_db  # 你自己的MiniDB客户端对象

def tx_start(args: Dict[str, Any]) -> Dict[str, Any]:
    try:
        tx_id = mini_db.start()
        return {"ok": True, "tx_id": tx_id}
    except Exception as e:
        return {"ok": False, "error": f"start failed: {e}"}

def tx_get(args: Dict[str, Any]) -> Dict[str, Any]:
    tx_id = args.get("tx_id")
    key = args.get("key", "").strip()
    if not tx_id or not key:
        return {"ok": False, "error": "tx_id and key are required"}
    try:
        val = mini_db.get(tx_id, key)
        return {"ok": True, "key": key, "value": val}
    except Exception as e:
        return {"ok": False, "error": f"get failed: {e}"}

def tx_put(args: Dict[str, Any]) -> Dict[str, Any]:
    tx_id = args.get("tx_id")
    key = args.get("key", "").strip()
    value = args.get("value")
    if not tx_id or not key:
        return {"ok": False, "error": "tx_id and key are required"}
    if not isinstance(value, str):
        import json
        value = json.dumps(value, ensure_ascii=False)
    try:
        mini_db.put(tx_id, key, value)
        return {"ok": True, "key": key}
    except Exception as e:
        return {"ok": False, "error": f"put failed: {e}"}

def tx_commit(args: Dict[str, Any]) -> Dict[str, Any]:
    tx_id = args.get("tx_id")
    if not tx_id:
        return {"ok": False, "error": "tx_id is required"}
    try:
        mini_db.commit(tx_id)
        return {"ok": True, "tx_id": tx_id}
    except Exception as e:
        return {"ok": False, "error": f"commit failed: {e}"}

def tx_rollback(args: Dict[str, Any]) -> Dict[str, Any]:
    tx_id = args.get("tx_id")
    if not tx_id:
        return {"ok": False, "error": "tx_id is required"}
    try:
        mini_db.rollback(tx_id)
        return {"ok": True, "tx_id": tx_id}
    except Exception as e:
        return {"ok": False, "error": f"rollback failed: {e}"}
