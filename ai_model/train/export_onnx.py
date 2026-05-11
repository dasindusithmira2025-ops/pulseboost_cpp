"""
Export PulseModel to ONNX and apply dynamic quantization.
"""

from __future__ import annotations

import os
from pathlib import Path

from onnxruntime.quantization import QuantType, quantize_dynamic
from optimum.exporters.onnx import main_export


FINETUNED_MODEL_PATH = Path("./ai_model/models/pulseModel-v1")
ONNX_OUTPUT_PATH = Path("./ai_model/models/pulseModel-v1-onnx")
QUANTIZED_OUTPUT = Path("./ai_model/models/pulseBoost-v1.onnx")


def main() -> None:
    if not FINETUNED_MODEL_PATH.exists():
        raise SystemExit("Fine-tuned model is missing. Run fine_tune.py first.")

    ONNX_OUTPUT_PATH.mkdir(parents=True, exist_ok=True)
    print("Exporting ONNX...")
    main_export(
        model_name_or_path=str(FINETUNED_MODEL_PATH),
        output=str(ONNX_OUTPUT_PATH),
        task="text-generation-with-past",
        device="cuda",
        fp16=True,
        optimize="O3",
    )

    source_model = ONNX_OUTPUT_PATH / "model.onnx"
    if not source_model.exists():
        raise SystemExit("ONNX export failed: model.onnx not found")

    print("Quantizing...")
    quantize_dynamic(
        model_input=str(source_model),
        model_output=str(QUANTIZED_OUTPUT),
        weight_type=QuantType.QInt8,
        optimize_model=True,
    )

    size_mb = os.path.getsize(QUANTIZED_OUTPUT) / (1024 * 1024)
    print(f"Exported: {QUANTIZED_OUTPUT} ({size_mb:.1f} MB)")


if __name__ == "__main__":
    main()

