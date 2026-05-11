"""
PulseModel fine-tuning script.

Base: microsoft/Phi-3-mini-4k-instruct
Method: QLoRA
"""

from __future__ import annotations

import torch
from datasets import load_dataset
from peft import LoraConfig, TaskType, get_peft_model, prepare_model_for_kbit_training
from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig, TrainingArguments
from trl import SFTTrainer


BASE_MODEL = "microsoft/Phi-3-mini-4k-instruct"
DATASET_PATH = "./ai_model/data/merged_dataset.jsonl"
OUTPUT_DIR = "./ai_model/models/pulseModel-v1"
MAX_SEQ_LENGTH = 2048


def format_training_example(example: dict) -> str:
    messages = example.get("messages", [])
    prompt = ""
    for msg in messages:
        role = msg.get("role", "user")
        content = msg.get("content", "")
        prompt += f"<|{role}|>\n{content}<|end|>\n"
    return prompt


def main() -> None:
    bnb_config = BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_compute_dtype=torch.bfloat16,
        bnb_4bit_use_double_quant=True,
    )

    lora_config = LoraConfig(
        task_type=TaskType.CAUSAL_LM,
        r=16,
        lora_alpha=32,
        lora_dropout=0.05,
        target_modules=["q_proj", "k_proj", "v_proj", "o_proj", "gate_proj", "up_proj", "down_proj"],
        bias="none",
    )

    training_args = TrainingArguments(
        output_dir=OUTPUT_DIR,
        num_train_epochs=3,
        per_device_train_batch_size=4,
        gradient_accumulation_steps=4,
        gradient_checkpointing=True,
        optim="paged_adamw_32bit",
        learning_rate=2e-4,
        weight_decay=0.001,
        bf16=True,
        logging_steps=10,
        save_steps=100,
        eval_strategy="steps",
        eval_steps=50,
        lr_scheduler_type="cosine",
        warmup_ratio=0.03,
        max_grad_norm=0.3,
        report_to="tensorboard",
    )

    tokenizer = AutoTokenizer.from_pretrained(BASE_MODEL, trust_remote_code=True)
    tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"

    model = AutoModelForCausalLM.from_pretrained(
        BASE_MODEL,
        quantization_config=bnb_config,
        device_map="auto",
        trust_remote_code=True,
        torch_dtype=torch.bfloat16,
    )
    model = prepare_model_for_kbit_training(model)
    model = get_peft_model(model, lora_config)

    dataset = load_dataset("json", data_files=DATASET_PATH, split="train")
    dataset = dataset.map(lambda row: {"text": format_training_example(row)})
    dataset = dataset.train_test_split(test_size=0.1, seed=42)

    trainer = SFTTrainer(
        model=model,
        train_dataset=dataset["train"],
        eval_dataset=dataset["test"],
        dataset_text_field="text",
        tokenizer=tokenizer,
        max_seq_length=MAX_SEQ_LENGTH,
        args=training_args,
        packing=False,
    )

    trainer.train()
    trainer.save_model(OUTPUT_DIR)
    tokenizer.save_pretrained(OUTPUT_DIR)
    print("Saved fine-tuned model to", OUTPUT_DIR)


if __name__ == "__main__":
    main()

