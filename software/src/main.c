/******************************************************************************
 *
 * FIR filter demo: configure custom FIR over AXI4-Lite, stream data through
 * PL BRAMs via AXI DMA (Simple mode, polling). No PS DDR or cache maintenance.
 *
 * Hardware platform: fir_demo_wrapper.xsa
 *   axi_bram_dst  @ 0xA0000000  (dest BRAM, PS read / DMA S2MM)
 *   axi_bram_src  @ 0xA0002000  (src BRAM ROM, DMA MM2S)
 *   axi_dma_0     @ 0xA0010000  (Simple DMA)
 *   fir_top_0     @ 0xA0020000  (FIR control)
 *
 * Source BRAM is loaded at runtime via XSCT before the demo runs (see prompt).
 *
 ******************************************************************************/

#include "platform.h"
#include "hw_config.h"
#include "xaxidma.h"
#include "xaxidma_hw.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xtime_l.h"

#define FIR_NUM_TAPS         5U
#define FIR_CTRL_OFFSET      0x00U
#define FIR_COEF_1_OFFSET    0x04U
#define FIR_EN_BIT           0x1U
#define RESULT_COUNT         10U
#define TRANSFER_SAMPLE_COUNT  (TRANSFER_LEN_BYTES / 4U)

typedef struct {
	u64 start_ticks;
	u64 s2mm_done_ticks;
	u64 mm2s_done_ticks;
} AccelTiming;

static const s16 FirCoeffs[FIR_NUM_TAPS] = { 100, 200, 300, 200, 100 };

static u64 TicksToUs(u64 ticks)
{
	return (ticks * 1000000ULL) / (u64)COUNTS_PER_SECOND;
}

static void PrintAccelTiming(const AccelTiming *t)
{
	u64 end_ticks = (t->s2mm_done_ticks > t->mm2s_done_ticks) ?
			t->s2mm_done_ticks : t->mm2s_done_ticks;
	u64 exec_ticks = end_ticks - t->start_ticks;
	u64 exec_us = TicksToUs(exec_ticks);
	u64 s2mm_us = TicksToUs(t->s2mm_done_ticks - t->start_ticks);
	u64 mm2s_us = TicksToUs(t->mm2s_done_ticks - t->start_ticks);
	u64 samples_per_s;

	xil_printf("Accelerator timing (%u samples, %u bytes):\r\n",
		   TRANSFER_SAMPLE_COUNT, TRANSFER_LEN_BYTES);
	xil_printf("  MM2S done: %llu us\r\n", (unsigned long long)mm2s_us);
	xil_printf("  S2MM done: %llu us\r\n", (unsigned long long)s2mm_us);
	xil_printf("  Execution: %llu us (%llu.%03llu ms)\r\n",
		   (unsigned long long)exec_us,
		   (unsigned long long)(exec_us / 1000ULL),
		   (unsigned long long)(exec_us % 1000ULL));

	if (exec_us > 0ULL) {
		samples_per_s = ((u64)TRANSFER_SAMPLE_COUNT * 1000000ULL) / exec_us;
		xil_printf("  Throughput: %llu samples/s\r\n",
			   (unsigned long long)samples_per_s);
	}
}

static void WaitForStimulusUpload(void)
{
	char c;
	u32 first;

	xil_printf("\r\n=== Stimulus upload required (XSCT) ===\r\n");
	xil_printf("Load %u words (%u bytes) into src BRAM before continuing.\r\n",
		   TRANSFER_SAMPLE_COUNT, TRANSFER_LEN_BYTES);
	xil_printf("  Target address: 0x%08X\r\n", SRC_BRAM_BASEADDR);
	xil_printf("  Init file:      stimulus/src_stimulus.coe (radix 16)\r\n");
	xil_printf("\r\n");
	xil_printf("  xsct> connect\r\n");
	xil_printf("  xsct> targets -set <a53_core>\r\n");
	xil_printf("  xsct> source <path>/load_stimulus.tcl\r\n");
	xil_printf("\r\n");
	xil_printf("Use 0x prefix for hex COE values, e.g. mwr 0x%08X 0x00000001\r\n",
		   SRC_BRAM_BASEADDR);
	xil_printf("Press ENTER in this UART window when upload is complete...\r\n");

	do {
		c = inbyte();
	} while (c != '\r' && c != '\n');

	first = Xil_In32(SRC_BRAM_BASEADDR);
	xil_printf("src BRAM[0] = 0x%08X\r\n", first);
	if (first == 0U) {
		xil_printf("Warning: first stimulus word is 0 - check XSCT upload\r\n");
	}

	xil_printf("Starting FIR DMA demo...\r\n\r\n");
}

static void ConfigureFirFilter(void)
{
	u32 i;

	for (i = 0; i < FIR_NUM_TAPS; i++) {
		Xil_Out32(CUSTOM_FIR_BASEADDR + FIR_COEF_1_OFFSET + (i * 4U),
			  (u32)FirCoeffs[i]);
	}

	Xil_Out32(CUSTOM_FIR_BASEADDR + FIR_CTRL_OFFSET, FIR_EN_BIT);
}

static void ReadBackFirCoeffs(void)
{
	u32 i;
	u32 ctrl;

	ctrl = Xil_In32(CUSTOM_FIR_BASEADDR + FIR_CTRL_OFFSET);
	xil_printf("FIR ctrl = 0x%08X (EN=%u)\r\n", ctrl, ctrl & FIR_EN_BIT);

	for (i = 0; i < FIR_NUM_TAPS; i++) {
		s16 readback = (s16)Xil_In32(CUSTOM_FIR_BASEADDR + FIR_COEF_1_OFFSET
					     + (i * 4U));
		xil_printf("coef[%u] wrote %d, read %d\r\n", i, FirCoeffs[i],
			   readback);
	}
}

static int InitAxiDma(XAxiDma *Inst)
{
	XAxiDma_Config *CfgPtr;
	int Status;

	CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (CfgPtr == NULL) {
		xil_printf("DMA lookup config failed\r\n");
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(Inst, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("DMA cfg initialize failed\r\n");
		return XST_FAILURE;
	}

	if (XAxiDma_HasSg(Inst)) {
		xil_printf("DMA is not in Simple mode\r\n");
		return XST_FAILURE;
	}

	XAxiDma_IntrDisable(Inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(Inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	return XST_SUCCESS;
}

static int RunDmaTransfer(XAxiDma *Inst, AccelTiming *timing)
{
	u32 Status;
	XTime now;

	XTime_GetTime(&now);
	timing->start_ticks = (u64)now;

	Status = XAxiDma_SimpleTransfer(Inst, DEST_BRAM_BASEADDR,
					TRANSFER_LEN_BYTES,
					XAXIDMA_DEVICE_TO_DMA);
	if (Status != XST_SUCCESS) {
		xil_printf("S2MM transfer start failed\r\n");
		return XST_FAILURE;
	}

	Status = XAxiDma_SimpleTransfer(Inst, SRC_BRAM_BASEADDR,
					TRANSFER_LEN_BYTES,
					XAXIDMA_DMA_TO_DEVICE);
	if (Status != XST_SUCCESS) {
		xil_printf("MM2S transfer start failed\r\n");
		return XST_FAILURE;
	}

	while (XAxiDma_Busy(Inst, XAXIDMA_DEVICE_TO_DMA)) {
		/* poll */
	}

	XTime_GetTime(&now);
	timing->s2mm_done_ticks = (u64)now;

	while (XAxiDma_Busy(Inst, XAXIDMA_DMA_TO_DEVICE)) {
		/* poll */
	}

	XTime_GetTime(&now);
	timing->mm2s_done_ticks = (u64)now;

	return XST_SUCCESS;
}

static void PrintResults(void)
{
	volatile u32 *dest = (volatile u32 *)DEST_BRAM_BASEADDR;
	u32 i;

	for (i = 0; i < RESULT_COUNT; i++) {
		xil_printf("result[%u] = %d\r\n", i, (s32)dest[i]);
	}
}

int main(void)
{
	XAxiDma AxiDma;
	AccelTiming timing;
	int Status;

	init_platform();

	xil_printf("FIR DMA demo start\r\n");
	xil_printf("  FIR base 0x%08X\r\n", CUSTOM_FIR_BASEADDR);
	xil_printf("  DMA dev id %u, len %u bytes\r\n", DMA_DEV_ID, TRANSFER_LEN_BYTES);
	xil_printf("  Timer freq %lu Hz\r\n", COUNTS_PER_SECOND);

	WaitForStimulusUpload();

	ConfigureFirFilter();
	ReadBackFirCoeffs();

	Status = InitAxiDma(&AxiDma);
	if (Status != XST_SUCCESS) {
		cleanup_platform();
		return XST_FAILURE;
	}

	Status = RunDmaTransfer(&AxiDma, &timing);
	if (Status != XST_SUCCESS) {
		cleanup_platform();
		return XST_FAILURE;
	}

	PrintAccelTiming(&timing);
	PrintResults();

	xil_printf("FIR DMA demo done\r\n");

	cleanup_platform();
	return XST_SUCCESS;
}
