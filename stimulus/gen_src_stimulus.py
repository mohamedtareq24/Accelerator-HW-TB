#!/usr/bin/env python3
"""Generate src_stimulus.coe and src_stimulus.mem for axi_bram_src_bram init."""

from pathlib import Path

COUNT = 256
START = 1
COE_OUTPUT = Path(__file__).with_name("src_stimulus.coe")
MEM_OUTPUT = Path(__file__).with_name("src_stimulus.mem")


def hex_word(value: int) -> str:
    return f"{value:08X}"


def samples() -> list[int]:
    return list(range(START, START + COUNT))


def write_coe(path: Path, values: list[int]) -> None:
    lines = [f"{hex_word(v)}," for v in values[:-1]]
    lines.append(f"{hex_word(values[-1])};")

    path.write_text(
        "memory_initialization_radix=16;\n"
        "memory_initialization_vector=\n"
        + "\n".join(lines)
        + "\n",
        encoding="utf-8",
    )


def write_mem(path: Path, values: list[int]) -> None:
    # Xilinx .mem: start address then one 32-bit hex word per line
    lines = ["@00000000"] + [hex_word(v) for v in values]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    values = samples()
    write_coe(COE_OUTPUT, values)
    write_mem(MEM_OUTPUT, values)
    print(f"Wrote {len(values)} samples to {COE_OUTPUT}")
    print(f"Wrote {len(values)} samples to {MEM_OUTPUT}")


if __name__ == "__main__":
    main()
