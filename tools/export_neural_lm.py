"""Export the trained character GRU model (tools/train_neural_lm.py) for the IME.

Binary layout (little endian):
  magic "LPNN", u32 version (1), u32 vocab V, u32 embed E, u32 hidden H, u32 0,
  u16 characters[V] (the UTF-16 unit of each token; 0 for the pad, unknown,
  clause-start and clause-end tokens 0-3 and for characters outside the BMP),
  then IEEE half floats: embedding[V][E], gru_input[3H][E], gru_hidden[3H][H],
  gru_input_bias[3H], gru_hidden_bias[3H], projection[E][H], projection_bias[E],
  output_bias[V]. Gates are ordered reset, update, new as in PyTorch; output
  logits are embedding . (projection h + projection_bias) + output_bias.

  python tools/export_neural_lm.py model.pt resources/dictionary/neural_zh.lm
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np
import torch

MAGIC = b"LPNN"
VERSION = 1


def main() -> None:
    source, target = Path(sys.argv[1]), Path(sys.argv[2])
    checkpoint = torch.load(source, map_location="cpu")
    state = {key: value.float().numpy() for key, value in checkpoint["state"].items()}
    vocab, embed, hidden = checkpoint["vocab"], checkpoint["embed"], checkpoint["hidden"]
    chars = [0, 0, 0, 0] + [ord(ch) if len(ch) == 1 and ord(ch) < 0x10000 else 0 for ch in checkpoint["chars"]]
    chars = (chars + [0] * vocab)[:vocab]
    arrays = [state["embed.weight"], state["rnn.weight_ih_l0"], state["rnn.weight_hh_l0"], state["rnn.bias_ih_l0"],
              state["rnn.bias_hh_l0"], state["proj.weight"], state["proj.bias"], state["bias"]]
    shapes = [(vocab, embed), (3 * hidden, embed), (3 * hidden, hidden), (3 * hidden,), (3 * hidden,),
              (embed, hidden), (embed,), (vocab,)]
    for array, shape in zip(arrays, shapes):
        assert array.shape == shape, (array.shape, shape)
    with target.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(struct.pack("<IIIII", VERSION, vocab, embed, hidden, 0))
        handle.write(np.asarray(chars, dtype="<u2").tobytes())
        for array in arrays:
            handle.write(array.astype("<f2").tobytes())
    metadata = {"format": "LPNN", "format_version": VERSION, "vocab": vocab, "embed": embed, "hidden": hidden,
                "parameters": int(sum(a.size for a in arrays)), "source_checkpoint_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                **{key: checkpoint[key] for key in ("training",) if key in checkpoint},
                "sha256": hashlib.sha256(target.read_bytes()).hexdigest()}
    target.with_name(target.name + ".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
                                                                encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("vocab", "embed", "hidden", "parameters")}), target.stat().st_size)


if __name__ == "__main__":
    main()
