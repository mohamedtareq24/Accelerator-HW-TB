/******************************************************************************
 *
 * Minimal stdin/UART RX test for Vitis standalone (Zynq UltraScale+).
 * Use this to verify keyboard input reaches inbyte()/getchar().
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"

static void ReadLine(char *buf, int max_len)
{
	int i = 0;
	char c;

	while (i < max_len - 1) {
		c = inbyte();
		if (c == '\r' || c == '\n') {
			break;
		}
		buf[i++] = c;
		outbyte(c); /* local echo */
	}
	buf[i] = '\0';
	xil_printf("\r\n");
}

int main(void)
{
	char c;
	char line[64];

	init_platform();

	xil_printf("Hello World\r\n");
	xil_printf("stdin test — use the Vitis Serial Terminal (115200 8N1)\r\n\r\n");

	xil_printf("Test 1: single key (inbyte)\r\n");
	xil_printf("Press one key: ");
	c = inbyte();
	xil_printf("\r\n  received: '%c' (0x%02X)\r\n\r\n", c, (unsigned char)c);

	xil_printf("Test 2: line until Enter (inbyte loop)\r\n");
	xil_printf("Type a line and press Enter: ");
	ReadLine(line, (int)sizeof(line));
	xil_printf("  line: \"%s\" (len %u)\r\n\r\n", line, (unsigned int)strlen(line));

	xil_printf("Test 3: getchar()\r\n");
	xil_printf("Press one key: ");
	c = (char)getchar();
	xil_printf("\r\n  getchar: '%c' (0x%02X)\r\n\r\n", c, (unsigned char)c);

	xil_printf("stdin test done\r\n");

	cleanup_platform();
	return 0;
}
