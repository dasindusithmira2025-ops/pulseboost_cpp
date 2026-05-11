"""
Basic post-training evaluation helper.
"""

from __future__ import annotations

import json
from pathlib import Path

from transformers import AutoModelForCausalLM, AutoTokenizer, pipeline


MODEL_DIR = Path("./ai_model/models/pulseModel-v1")
EVAL_SET = Path("./ai_model/data/pc_diagnostics.jsonl")


def load_prompts(limit: int = 20) -> list[str]:
    prompts: list[str] = []
    if not EVAL_SET.exists():
        return prompts
    with EVAL_SET.open("r", encoding="utf-8") as f:
        for line in f:
            row = json.loads(line)
            for msg in row.get("messages", []):
                if msg.get("role") == "user":
                    prompts.append(msg.get("content", ""))
                    break
            if len(prompts) >= limit:
                break
    return prompts


def main() -> None:
    if not MODEL_DIR.exists():
        raise SystemExit("Model directory does not exist. Run fine_tune.py first.")

    tokenizer = AutoTokenizer.from_pretrained(MODEL_DIR, trust_remote_code=True)
    model = AutoModelForCausalLM.from_pretrained(MODEL_DIR, trust_remote_code=True)
    generator = pipeline("text-generation", model=model, tokenizer=tokenizer)

    prompts = load_prompts()
    if not prompts:
        raise SystemExit("No eval prompts found.")

    keyword_hits = 0
    for idx, prompt in enumerate(prompts, start=1):
        out = generator(prompt, max_new_tokens=160, do_sample=True, temperature=0.4)[0]["generated_text"]
        text = out.lower()
        if any(token in text for token in ["restore point", "startup", "ram", "disk", "safe"]):
            keyword_hits += 1
        print(f"[{idx:02d}] OK")

    print(f"Keyword coverage: {keyword_hits}/{len(prompts)}")


if __name__ == "__main__":
    main()

