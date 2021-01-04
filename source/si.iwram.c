/* 
 * Copyright (c) 2016-2021, Extrems' Corner.org
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include "bios.h"

void SISetResponse(const void *buf, unsigned bits)
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
				"strb  %1, %2 \n"
				"nop \n"
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
				"strb  %1, %2 \n"
				"nop \n"
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
		"strb  %1, %2 \n"
		:
		: "r" (GPIO_SO_IO), "r" (GPIO_SO_IO | GPIO_SO),
		  "m" (REG_RCNT)
		: "memory"
	);
}

int SIGetCommand(void *buf, unsigned bits)
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
