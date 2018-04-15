#include <stdint.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include "bios.h"

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
	unsigned irq;

	REG_TM2CNT_H = REG_TM0CNT_H = 0;
	REG_IF = irq = REG_IF;
	REG_TM2CNT_H = TIMER_START | TIMER_IRQ | 3;

	do {
		CustomHalt(irq & IRQ_TIMER2 ? STOP : HALT);
		REG_TM0CNT_H = 0;
		REG_IF = irq = REG_IF;
		REG_TM0CNT_H = TIMER_START | TIMER_IRQ;

		if (irq & IRQ_SERIAL) {
			byte <<= 1;
			byte |= !!((REG_RCNT | REG_RCNT | REG_RCNT) & GPIO_SI);

			if (++bit % 8 == 0)
				*(uint8_t *)buf++ = byte;
		} else if (irq & IRQ_TIMER0)
			break;
	} while (bit < bits);

	return bit;
}
