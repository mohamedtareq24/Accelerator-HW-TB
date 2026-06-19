# Zynq UltraScale+ FIR Accelerator Tutorial

**Self-contained student kit** for the Avnet Ultra96-V2 — everything needed for the lab is in this folder.

## Main idea

The custom **FIR block is a dummy accelerator** — it stands in for any PL IP you would offload to later (CNN layer, FFT, custom DSP, etc.). What matters for this lab is not the filter math, but the **accelerator pattern**:

| Interface | Role |
|-----------|------|
| **AXI4-Lite (`s_axi`)** | Control plane — PS writes coefficients and an enable bit before the run |
| **AXI-Stream (`s_axis` / `m_axis`)** | Datapath — samples stream in, filtered samples stream out |

The PS does **not** push samples through the FIR register-by-register. It configures the block, then **DMA moves bulk data** through the stream ports while the FIR runs in hardware.

## Data movement

Two paths run in parallel:

**Control (PS → accelerator)**  
PS writes to `fir_top_0` over AXI4-Lite: load tap coefficients, assert enable.

**Datapath (memory ↔ DMA ↔ FIR ↔ memory)**

```
axi_bram_src          axi_dma_0                                         axi_dma_0          axi_bram_dst
 (source mem)    ──MM2S AXI4 read──►  ──AXI-Stream──► | Accelerator |──AXI-Stream──►  ──S2MM AXI4 write──►  (dest mem)
 0xA0002000              │                             s_axis / m_axis                     0xA0000000
                         │
                    PS programs
                    transfer length
                    via AXI4-Lite
```

1. PS programs **AXI DMA** in **Simple mode** (scatter-gather disabled): byte count and source/dest addresses.
2. **MM2S** reads 32-bit words from **source BRAM** and pushes them on the FIR input stream.
3. The FIR consumes samples on `s_axis` and produces results on `m_axis`.
4. **S2MM** captures the output stream and writes **destination BRAM**.
5. PS reads results from destination BRAM and prints timing.

Software starts **S2MM first**, then **MM2S**, so the output path is ready before samples arrive.

Stimulus and results sit in **PL BRAM**, not PS DDR — so this demo avoids cache maintenance on the datapath.

## How are we going to seed the source memory?

Source BRAM (`axi_bram_src` @ `0xA0002000`) holds the input samples DMA will read. Before the transfer can mean anything, that memory must contain valid data.

**Question for the lab:** *How do we get 256 stimulus words into source BRAM before kicking off DMA?*

Options to think about:

- Initialize it in the **bitstream** (COE/MIF at synthesis)?
- Have **`main.c` copy** a buffer into BRAM over the PS AXI port?
- Load it **from outside** while the CPU is running (debugger / XSCT)?

This kit deliberately **does not** bake stimulus into the shipped bitstream. The app pauses on UART and expects you to seed source BRAM at runtime — see [docs/02-software.md — Launch on board](docs/02-software.md#launch-on-board-main--xsct) for the chosen approach (`load_stimulus.tcl` + `src_stimulus.coe`).

## Kit layout

```
Accelerator-HW-TB/
├── README.md
├── docs/
│   ├── 01-block-design.md
│   └── 02-software.md
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
| 2. Software + seed src mem + run | [docs/02-software.md](docs/02-software.md) |

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
