# Zynq UltraScale+ FIR Accelerator Tutorial

**Self-contained student kit** — everything needed for the Ultra96-V2 lab is in this folder.

Bare-metal tutorial: custom 5-tap FIR in PL, AXI DMA streaming, BRAM stimulus/results, PS control over AXI4-Lite, XSCT stimulus upload, and execution-time measurement.

## Kit layout

```
ultimate/
├── README.md
├── docs/
│   ├── 01-block-design.md
│   ├── 02-software.md
│   └── 03-execution.md
├── hardware/
│   ├── platform/fir_demo_wrapper.xsa
│   ├── rtl/
│   └── scripts/create_bd.tcl
├── software/src/
│   ├── main.c
│   └── hw_config.h
└── stimulus/
    ├── src_stimulus.coe
    ├── src_stimulus.mem
    ├── gen_src_stimulus.py
    └── load_stimulus.tcl
```

## Quick start

| Step | Document |
|------|----------|
| 1. Block design | [docs/01-block-design.md](docs/01-block-design.md) |
| 2. Software | [docs/02-software.md](docs/02-software.md) |
| 3. Run on board | [docs/03-execution.md](docs/03-execution.md) |

## Requirements

- **Board:** Avnet Ultra96-V2 (`xczu3eg`)
- **Tools:** Vitis / SDK 2020.1+ (XSA from Vivado 2023.1)
- **Cable:** USB-JTAG/UART

## Memory map

| Peripheral | Address | Role |
|------------|---------|------|
| `axi_bram_dst` | `0xA0000000` | Filter output |
| `axi_bram_src` | `0xA0002000` | Stimulus (XSCT load) |
| `axi_dma_0` | `0xA0010000` | Simple DMA |
| `fir_top_0` | `0xA0020000` | FIR control |

**Datapath:** src BRAM → DMA MM2S → FIR → DMA S2MM → dest BRAM
