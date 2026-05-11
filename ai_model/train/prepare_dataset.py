"""
Merge all JSONL sources into ai_model/data/merged_dataset.jsonl.
"""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
OUT_PATH = DATA_DIR / "merged_dataset.jsonl"

SOURCES = [
    DATA_DIR / "base_dataset.jsonl",
    DATA_DIR / "pc_diagnostics.jsonl",
    DATA_DIR / "optimization_responses.jsonl",
    DATA_DIR / "safety_responses.jsonl",
]


def load_jsonl(path: Path) -> list[dict]:
    if not path.exists():
        return []
    rows: list[dict] = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            rows.append(json.loads(line))
    return rows


def main() -> None:
    merged: list[dict] = []
    for source in SOURCES:
        rows = load_jsonl(source)
        print(f"{source.name}: {len(rows)}")
        merged.extend(rows)

    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUT_PATH.open("w", encoding="utf-8") as f:
        for row in merged:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    print(f"merged_dataset.jsonl: {len(merged)} rows")


if __name__ == "__main__":
    main()

