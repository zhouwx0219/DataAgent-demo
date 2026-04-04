# Agent/tool_registry.py
from typing import Dict, Any
from tools.kv_tx_tools import (
    kv_get, tx_start, tx_get, tx_put, tx_commit, tx_rollback, kv_dump
)
from tools.image_tools import image_analyze


TOOLS_SCHEMA = [
    # ---------- KV / 事务 ----------
    {
        "type": "function",
        "function": {
            "name": "kv_get",
            "description": "只读查询某个key的value（无事务）",
            "parameters": {
                "type": "object",
                "properties": {
                    "key": {"type": "string"}
                },
                "required": ["key"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "tx_start",
            "description": "开启事务，返回tx_id",
            "parameters": {
                "type": "object",
                "properties": {},
                "required": []
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "tx_get",
            "description": "事务内读取key",
            "parameters": {
                "type": "object",
                "properties": {
                    "tx_id": {"type": "string"},
                    "key": {"type": "string"}
                },
                "required": ["tx_id", "key"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "tx_put",
            "description": "事务内写入key/value",
            "parameters": {
                "type": "object",
                "properties": {
                    "tx_id": {"type": "string"},
                    "key": {"type": "string"},
                    "value": {"type": "string"}
                },
                "required": ["tx_id", "key", "value"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "tx_commit",
            "description": "提交事务",
            "parameters": {
                "type": "object",
                "properties": {
                    "tx_id": {"type": "string"}
                },
                "required": ["tx_id"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "tx_rollback",
            "description": "回滚事务",
            "parameters": {
                "type": "object",
                "properties": {
                    "tx_id": {"type": "string"}
                },
                "required": ["tx_id"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "kv_dump",
            "description": "调试：查看当前所有KV数据",
            "parameters": {
                "type": "object",
                "properties": {},
                "required": []
            }
        }
    },

    # ---------- 图片 ----------
    {
        "type": "function",
        "function": {
            "name": "image_analyze",
            "description": "分析图片（占位版）",
            "parameters": {
                "type": "object",
                "properties": {
                    "image_path": {"type": "string"},
                    "task": {
                        "type": "string",
                        "enum": ["describe", "ocr", "detect"]
                    }
                },
                "required": ["image_path"]
            }
        }
    }
]


def tool_router(name: str, args: Dict[str, Any]) -> Dict[str, Any]:
    if name == "kv_get":
        return kv_get(args)
    if name == "tx_start":
        return tx_start(args)
    if name == "tx_get":
        return tx_get(args)
    if name == "tx_put":
        return tx_put(args)
    if name == "tx_commit":
        return tx_commit(args)
    if name == "tx_rollback":
        return tx_rollback(args)
    if name == "kv_dump":
        return kv_dump(args)
    if name == "image_analyze":
        return image_analyze(args)

    return {"ok": False, "error": f"unknown tool: {name}"}
