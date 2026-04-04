# Agent/llm.py
import os
import json
from typing import List, Dict, Any
from dotenv import load_dotenv
from openai import OpenAI

load_dotenv()

client = OpenAI(
    api_key=os.getenv("OPENAI_API_KEY"),
    base_url=os.getenv("OPENAI_BASE_URL"),
)

MODEL_NAME = os.getenv("MODEL_NAME", "qwen-plus")


def chat_with_tools(
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]],
        tool_router,
        max_rounds: int = 10
) -> str:
    for _ in range(max_rounds):
        resp = client.chat.completions.create(
            model=MODEL_NAME,
            messages=messages,
            tools=tools,
            tool_choice="auto",
            temperature=0.2,
        )

        msg = resp.choices[0].message

        # 有工具调用
        if msg.tool_calls:
            messages.append({
                "role": "assistant",
                "content": msg.content or "",
                "tool_calls": [
                    {
                        "id": tc.id,
                        "type": tc.type,
                        "function": {
                            "name": tc.function.name,
                            "arguments": tc.function.arguments
                        }
                    } for tc in msg.tool_calls
                ]
            })

            for tc in msg.tool_calls:
                fn_name = tc.function.name
                raw_args = tc.function.arguments or "{}"

                try:
                    fn_args = json.loads(raw_args)
                except Exception:
                    fn_args = {}

                result = tool_router(fn_name, fn_args)

                messages.append({
                    "role": "tool",
                    "tool_call_id": tc.id,
                    "name": fn_name,
                    "content": json.dumps(result, ensure_ascii=False)
                })
            continue

        # 无工具调用，返回文本
        return msg.content or ""

    return "达到最大工具调用轮次，流程未完成。"
