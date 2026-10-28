"""Train the character GRU model that rescores sentence candidates (neural_zh.lm).

Training text is the context model's (tools/fetch_context_corpus.py --work),
weighted the same way; clauses containing any evaluation sentence of four or
more characters are skipped exactly as for the n-gram models, and so are the
word-level and everyday-chat evaluation files. Only weights are packaged
(tools/export_neural_lm.py); no text is.

Authoring-time requirements: PyTorch with CUDA, numpy.

  python tools/train_neural_lm.py --work <dir> --output model.pt [--hidden 768] [--embed 256]
                                  [--vocab 8000] [--epochs 2]
  python tools/export_neural_lm.py model.pt resources/dictionary/neural_zh.lm
"""
from __future__ import annotations

import argparse
import math
import sys
import time
from collections import Counter
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).parent))
from build_context_lm import HAN_RUN, excluded_texts  # noqa: E402

PAD, UNK, BOS, EOS = 0, 1, 2, 3
SEQ, BATCH = 128, 128
EXCLUDE = ["typing_holdout.tsv", "typing_fresh.tsv", "typing_independent.tsv", "scenario_replay_076.tsv",
           "common_word_order_079.tsv", "heldout_sentences.tsv", "heldout_dialogue_sentences.tsv",
           "heldout_context.tsv", "heldout_dialogue_context.tsv", "colloquial_dev.tsv", "colloquial_test.tsv"]
WEIGHTS = {"wikinews": 2, "tatoeba": 3, "cv": 3, "kdconv": 3, "crosswoz": 3, "fineweb": 1, "fineweb_extra": 1, "fineweb_extra2": 1}


class CharLM(nn.Module):
    def __init__(self, vocab: int, embed: int, hidden: int) -> None:
        super().__init__()
        self.embed = nn.Embedding(vocab, embed)
        self.rnn = nn.GRU(embed, hidden, batch_first=True)
        self.proj = nn.Linear(hidden, embed)
        self.bias = nn.Parameter(torch.zeros(vocab))

    def forward(self, tokens, state=None):
        out, state = self.rnn(self.embed(tokens), state)
        return self.proj(out) @ self.embed.weight.t() + self.bias, state


def training_clauses(work: Path) -> tuple[list[str], list[tuple[Path, int, int]]]:
    authoring = ROOT / "resources/dictionary/authoring"
    sources = [(p, 8 if p.name.startswith("daily_") else 1) for p in sorted(authoring.glob("*.txt"))]
    sources += [(work / "train" / f"{name}.txt", weight) for name, weight in WEIGHTS.items()]
    by_first: dict[str, list[str]] = {}
    for item in excluded_texts([ROOT / "tests/data" / name for name in EXCLUDE]):
        by_first.setdefault(item[0], []).append(item)
    clauses, manifest = [], []
    for path, weight in sources:
        count = 0
        for line in path.read_text(encoding="utf-8").splitlines():
            for clause in HAN_RUN.findall(line):
                if len(clause) < 2 or any(item in clause for item in {i for ch in set(clause) for i in by_first.get(ch, ())}):
                    continue
                clauses.extend([clause] * weight)
                count += 1
        manifest.append((path, weight, count))
        print(path.name, count, "x", weight, flush=True)
    return clauses, manifest


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--hidden", type=int, default=768)
    parser.add_argument("--embed", type=int, default=256)
    parser.add_argument("--vocab", type=int, default=8000)
    parser.add_argument("--epochs", type=float, default=2.0)
    args = parser.parse_args()
    torch.manual_seed(0)
    clauses, manifest = training_clauses(args.work)
    counts = Counter(ch for clause in clauses for ch in clause)
    chars = [ch for ch, _ in counts.most_common(args.vocab - 4)]
    lookup = np.full(0x10000, UNK, dtype=np.uint16)
    for index, ch in enumerate(chars):
        if ord(ch) < 0x10000:
            lookup[ord(ch)] = index + 4
    codes = np.frombuffer("\n".join(clauses).encode("utf-16-le"), dtype=np.uint16)
    stream = lookup[codes]
    stream[codes == ord("\n")] = EOS
    stream = np.insert(stream, np.flatnonzero(stream == EOS) + 1, BOS)
    stream = np.concatenate([[BOS], stream, [EOS]]).astype(np.uint16)
    valid = torch.from_numpy(stream[-2_000_000:].astype(np.int64))
    train = stream[:-2_000_000]
    model = CharLM(args.vocab, args.embed, args.hidden).cuda()
    steps = int(args.epochs * len(train) / (SEQ * BATCH))
    optimizer = torch.optim.AdamW(model.parameters(), lr=2e-3, weight_decay=0.01)
    schedule = torch.optim.lr_scheduler.OneCycleLR(optimizer, max_lr=2e-3, total_steps=steps, pct_start=0.03)
    loss_fn = nn.CrossEntropyLoss()
    rng = np.random.default_rng(0)
    started = time.time()
    perplexity = float("nan")
    for step in range(steps):
        starts = rng.integers(0, len(train) - SEQ - 1, BATCH)
        batch = torch.from_numpy(np.stack([train[s:s + SEQ + 1] for s in starts]).astype(np.int64)).cuda()
        with torch.autocast("cuda", dtype=torch.bfloat16):
            logits, _ = model(batch[:, :-1])
            loss = loss_fn(logits.reshape(-1, args.vocab), batch[:, 1:].reshape(-1))
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()
        schedule.step()
        if step % 1000 == 0 or step == steps - 1:
            model.eval()
            with torch.no_grad(), torch.autocast("cuda", dtype=torch.bfloat16):
                chunk = valid[:SEQ * 128 + 1].cuda()
                vlogits, _ = model(chunk[:-1].reshape(128, SEQ))
                perplexity = math.exp(loss_fn(vlogits.reshape(-1, args.vocab), chunk[1:].reshape(128, SEQ).reshape(-1)).item())
            model.train()
            print(f"step {step}/{steps} loss {loss.item():.3f} valid perplexity {perplexity:.1f} "
                  f"{(step + 1) * SEQ * BATCH / (time.time() - started) / 1000:.0f}k tokens/s", flush=True)
    training = {"tokens": int(len(stream)), "steps": steps, "sequence": SEQ, "batch": BATCH, "epochs": args.epochs,
                "valid_perplexity": round(perplexity, 2),
                "inputs": [{"file": path.name, "weight": weight, "clauses": count} for path, weight, count in manifest]}
    torch.save({"state": model.state_dict(), "vocab": args.vocab, "embed": args.embed, "hidden": args.hidden,
                "chars": chars, "training": training}, args.output)
    print("saved", args.output, training["valid_perplexity"], flush=True)


if __name__ == "__main__":
    main()
