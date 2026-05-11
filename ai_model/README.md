# PulseModel Training Pipeline

This folder contains the local training and export pipeline for PulseModel.

## Goal

- Base model: `microsoft/Phi-3-mini-4k-instruct`
- Fine-tune method: QLoRA
- Export target: ONNX Runtime (CUDA or CPU fallback)
- Domain: Windows diagnostics, optimization actions, and safety-first refusal policy

## Quick Start

1. Install Python dependencies:

```powershell
python -m pip install -r ai_model/train/requirements.txt
```

2. Generate synthetic training data:

```powershell
python ai_model/train/collect_training_data.py
python ai_model/train/prepare_dataset.py
```

3. Fine-tune:

```powershell
python ai_model/train/fine_tune.py
```

4. Evaluate:

```powershell
python ai_model/train/evaluate_model.py
```

5. Export:

```powershell
python ai_model/train/export_onnx.py
```

## Notes

- Training requires a CUDA-capable GPU for practical speed.
- Exported ONNX model is expected at `ai_model/models/pulseBoost-v1.onnx`.
- Inference integration in C++ is optional and gated by `ONNXRUNTIME_DIR` in CMake.

