from typing import Dict, Any
import os

def image_analyze(args: Dict[str, Any]) -> Dict[str, Any]:
    image_path = args.get("image_path", "").strip()
    task = args.get("task", "describe").strip()

    if not image_path:
        return {"ok": False, "error": "image_path is required"}
    if not os.path.exists(image_path):
        return {"ok": False, "error": f"file not found: {image_path}"}

    # TODO: 这里替换为真实视觉模型调用（云端API或本地模型）
    return {
        "ok": True,
        "task": task,
        "caption": "这是一张示例图片（占位结果）",
        "objects": ["object_a", "object_b"]
    }
