/******************************************************************************
 * Hardware map extracted from fir_demo_wrapper.xsa (fir_demo.hwh).
 *
 * After importing the XSA into SDK/Vitis and regenerating the BSP, xparameters.h
 * supplies these values automatically. The fallbacks below keep the app buildable
 * until the BSP is refreshed.
 ******************************************************************************/

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include "xparameters.h"

/* AXI DMA (Simple mode, scatter-gather disabled) */
#ifdef XPAR_AXI_DMA_0_DEVICE_ID
#define DMA_DEV_ID           XPAR_AXI_DMA_0_DEVICE_ID
#else
#define DMA_DEV_ID           0
#endif

/* Destination BRAM (axi_bram_dst, PS port A / DMA S2MM port B) */
#if defined(XPAR_AXI_BRAM_DST_BASEADDR)
#define DEST_BRAM_BASEADDR   XPAR_AXI_BRAM_DST_BASEADDR
#elif defined(XPAR_BRAM_0_BASEADDR)
#define DEST_BRAM_BASEADDR   XPAR_BRAM_0_BASEADDR
#else
#define DEST_BRAM_BASEADDR   0xA0000000U
#endif

/* Source BRAM (axi_bram_src, ROM init via .coe, DMA MM2S) */
#if defined(XPAR_AXI_BRAM_SRC_BASEADDR)
#define SRC_BRAM_BASEADDR    XPAR_AXI_BRAM_SRC_BASEADDR
#elif defined(XPAR_BRAM_1_BASEADDR)
#define SRC_BRAM_BASEADDR    XPAR_BRAM_1_BASEADDR
#else
#define SRC_BRAM_BASEADDR    0xA0002000U
#endif

/* Custom FIR (fir_top_0 AXI4-Lite) */
#if defined(XPAR_FIR_TOP_0_BASEADDR)
#define CUSTOM_FIR_BASEADDR  XPAR_FIR_TOP_0_BASEADDR
#elif defined(XPAR_MY_FIR_0_BASEADDR)
#define CUSTOM_FIR_BASEADDR  XPAR_MY_FIR_0_BASEADDR
#else
#define CUSTOM_FIR_BASEADDR  0xA0020000U
#endif

/* 256 BRAM words x 4 bytes (matches stimulus/src_stimulus.coe) */
#define TRANSFER_LEN_BYTES   1024U

#endif /* HW_CONFIG_H */
