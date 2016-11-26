#include <stdint.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_systemcalls.h>

void IWRAM_CODE SISetResponse(const void *buf, unsigned bits)
{
	unsigned byte = 0, bit = 0;

	do {
		if (bit++ % 8 == 0)
			byte = *(uint8_t *)buf++;
		byte <<= 1;

		if (byte & 0x100) {
			asm volatile (
				"strb  %0, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %1, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %1, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %1, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				:
				: "r" (GPIO_SO_IO), "r" (GPIO_SO_IO | GPIO_SO),
				  "m" (REG_RCNT)
				: "memory"
			);
		} else {
			asm volatile (
				"strb  %0, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %0, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %0, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"nop \n"
				"strb  %1, %2 \n"
				"nop \n"
				"nop \n"
				"nop \n"
				:
				: "r" (GPIO_SO_IO), "r" (GPIO_SO_IO | GPIO_SO),
				  "m" (REG_RCNT)
				: "memory"
			);
		}
	} while (bit < bits);

	asm volatile (
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"strb  %0, %2 \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"nop \n"
		"strb  %1, %2 \n"
		:
		: "r" (GPIO_SO_IO), "r" (GPIO_SO_IO | GPIO_SO),
		  "m" (REG_RCNT)
		: "memory"
	);
}

int IWRAM_CODE SIGetCommand(void *buf, unsigned bits)
{
	unsigned byte = 0, bit = 0;

	REG_IF = REG_IF;
	Halt();

	do {
		register int a, b;
		asm volatile (
			"ldrb  %0, %2 \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"ldrb  %1, %2 \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
			: "=r" (a), "=r" (b)
			: "m" (REG_RCNT)
			: "memory"
		);

		byte <<= 1;
		byte |= !!(a & GPIO_SI);

		if (++bit % 8 == 0)
			*(uint8_t *)buf++ = byte;

		if (b & GPIO_SI) return bit;
	} while (bit < bits);

	return bit;
}
