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
#include <gba_dma.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include "bios.h"

#define struct struct __attribute__((packed, scalar_storage_order("big-endian")))

#define ROM           ((int16_t *)0x08000000)
#define ROM_GPIODATA *((int16_t *)0x080000C4)
#define ROM_GPIODIR  *((int16_t *)0x080000C6)
#define ROM_GPIOCNT  *((int16_t *)0x080000C8)

//#define ANALOG

enum {
	CMD_ID = 0x00,
	CMD_STATUS = 0x40,
	CMD_ORIGIN,
	CMD_RECALIBRATE,
	CMD_STATUS_LONG,
	CMD_RESET = 0xFF
};

enum {
	MOTOR_STOP = 0,
	MOTOR_RUMBLE,
	MOTOR_STOP_HARD
};

struct buttons {
	uint16_t            : 1;
	uint16_t unknown    : 1;
	uint16_t get_origin : 1;
	uint16_t start      : 1;
	uint16_t y          : 1;
	uint16_t x          : 1;
	uint16_t b          : 1;
	uint16_t a          : 1;
	uint16_t use_origin : 1;
	uint16_t l          : 1;
	uint16_t r          : 1;
	uint16_t z          : 1;
	uint16_t up         : 1;
	uint16_t down       : 1;
	uint16_t right      : 1;
	uint16_t left       : 1;
};

static struct {
	uint16_t type;
	struct {
		uint8_t            : 1;
		uint8_t unknown    : 1;
		uint8_t get_origin : 1;
		uint8_t motor      : 2;
		uint8_t mode       : 3;
	} status;
} id;

static struct {
	struct buttons buttons;
	struct { uint8_t x, y; } stick;
	union {
		struct {
			struct { uint8_t x : 8, y : 8; } substick;
			struct { uint8_t l : 4, r : 4; } trigger;
			struct { uint8_t a : 4, b : 4; } button;
		} mode0;

		struct {
			struct { uint8_t x : 4, y : 4; } substick;
			struct { uint8_t l : 8, r : 8; } trigger;
			struct { uint8_t a : 4, b : 4; } button;
		} mode1;

		struct {
			struct { uint8_t x : 4, y : 4; } substick;
			struct { uint8_t l : 4, r : 4; } trigger;
			struct { uint8_t a : 8, b : 8; } button;
		} mode2;

		struct {
			struct { uint8_t x, y; } substick;
			struct { uint8_t l, r; } trigger;
		} mode3;

		struct {
			struct { uint8_t x, y; } substick;
			struct { uint8_t a, b; } button;
		} mode4;
	};
} status;

static struct {
	struct buttons buttons;
	struct { uint8_t x, y; } stick;
	struct { uint8_t x, y; } substick;
	struct { uint8_t l, r; } trigger;
	struct { uint8_t a, b; } button;
} origin = {
	.buttons  = { .use_origin = 1 },
	.stick    = { 128, 128 },
	.substick = { 128, 128 },
};

static uint8_t buffer[128];

static enum {
	RUMBLE_NONE = 0,
	RUMBLE_GBA,
	RUMBLE_NDS,
	RUMBLE_NDS_SLIDE,
} rumble;

static bool has_motor(void)
{
	switch (ROM[0x59]) {
		case 0x59:
			switch (ROM[0xFFFFFF]) {
				case ~0x0002:
					rumble = RUMBLE_NDS;
					return true;
				case ~0x0101:
					rumble = RUMBLE_NDS_SLIDE;
					return true;
			}
			break;
		case 0x96:
			switch (ROM[0x56] & 0xFF) {
				case 'R':
				case 'V':
					rumble = RUMBLE_GBA;
					return true;
			}
			break;
	}

	rumble = RUMBLE_NONE;
	return false;
}

static void set_motor(bool enable)
{
	switch (rumble) {
		case RUMBLE_NONE:
			break;
		case RUMBLE_GBA:
			ROM_GPIODIR  =      1 << 3;
			ROM_GPIODATA = enable << 3;
			break;
		case RUMBLE_NDS:
			if (enable)
				DMA3COPY(SRAM, SRAM, DMA_VBLANK | DMA_REPEAT | 1)
			else
				REG_DMA3CNT &= ~DMA_REPEAT;
			break;
		case RUMBLE_NDS_SLIDE:
			*ROM = enable << 8;
			break;
	}
}

void SISetResponse(const void *buf, unsigned bits);
int SIGetCommand(void *buf, unsigned bits);

int main(void)
{
	RegisterRamReset(RESET_ALL_REG);

	REG_IE = IRQ_SERIAL | IRQ_TIMER2 | IRQ_TIMER1 | IRQ_TIMER0;
	REG_IF = REG_IF;

	REG_RCNT = R_GPIO | GPIO_IRQ | GPIO_SO_IO | GPIO_SO;

	REG_TM0CNT_L = -67;
	REG_TM1CNT_H = TIMER_START | TIMER_IRQ | TIMER_COUNT;
	REG_TM0CNT_H = TIMER_START;

	SoundBias(0);
	Halt();

	while (true) {
		int length = SIGetCommand(buffer, sizeof(buffer) * 8 + 1);
		if (length < 9) continue;

		unsigned buttons     = ~REG_KEYINPUT;
		origin.buttons.a     = !!(buttons & KEY_A);
		origin.buttons.b     = !!(buttons & KEY_B);
		origin.buttons.unknown = 
		origin.buttons.z     = !!(buttons & KEY_SELECT);
		origin.buttons.start = !!(buttons & KEY_START);
		#ifndef ANALOG
		origin.buttons.right = !!(buttons & KEY_RIGHT);
		origin.buttons.left  = !!(buttons & KEY_LEFT);
		origin.buttons.up    = !!(buttons & KEY_UP);
		origin.buttons.down  = !!(buttons & KEY_DOWN);
		#endif
		origin.buttons.r     = !!(buttons & KEY_R);
		origin.buttons.l     = !!(buttons & KEY_L);

		id.status.unknown = origin.buttons.unknown;

		switch (buffer[0]) {
			case CMD_RESET:
				id.status.motor = MOTOR_STOP;
			case CMD_ID:
				if (length == 9) {
					if (has_motor())
						id.type = 0x0900;
					else
						id.type = 0x2900;

					SISetResponse(&id, sizeof(id) * 8);
				}
				break;
			case CMD_STATUS:
				if (length == 25) {
					id.status.mode  = buffer[1];
					id.status.motor = buffer[2];

					status.buttons = origin.buttons;
					status.stick.x = origin.stick.x;
					status.stick.y = origin.stick.y;
					#ifdef ANALOG
					if (buttons & KEY_RIGHT)
						status.stick.x = origin.stick.x + 100;
					else if (buttons & KEY_LEFT)
						status.stick.x = origin.stick.x - 100;
					if (buttons & KEY_UP)
						status.stick.y = origin.stick.y + 100;
					else if (buttons & KEY_DOWN)
						status.stick.y = origin.stick.y - 100;
					#endif

					switch (id.status.mode) {
						default:
							status.mode0.substick.x = origin.substick.x;
							status.mode0.substick.y = origin.substick.y;
							status.mode0.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l) >> 4;
							status.mode0.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r) >> 4;
							status.mode0.button.a   = (buttons & KEY_A ? 200 : origin.button.a) >> 4;
							status.mode0.button.b   = (buttons & KEY_B ? 200 : origin.button.b) >> 4;
							break;
						case 1:
							status.mode1.substick.x = origin.substick.x >> 4;
							status.mode1.substick.y = origin.substick.y >> 4;
							status.mode1.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l);
							status.mode1.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r);
							status.mode1.button.a   = (buttons & KEY_A ? 200 : origin.button.a) >> 4;
							status.mode1.button.b   = (buttons & KEY_B ? 200 : origin.button.b) >> 4;
							break;
						case 2:
							status.mode2.substick.x = origin.substick.x >> 4;
							status.mode2.substick.y = origin.substick.y >> 4;
							status.mode2.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l) >> 4;
							status.mode2.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r) >> 4;
							status.mode2.button.a   = (buttons & KEY_A ? 200 : origin.button.a);
							status.mode2.button.b   = (buttons & KEY_B ? 200 : origin.button.b);
							break;
						case 3:
							status.mode3.substick.x = origin.substick.x;
							status.mode3.substick.y = origin.substick.y;
							status.mode3.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l);
							status.mode3.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r);
							break;
						case 4:
							status.mode4.substick.x = origin.substick.x;
							status.mode4.substick.y = origin.substick.y;
							status.mode4.button.a   = (buttons & KEY_A ? 200 : origin.button.a);
							status.mode4.button.b   = (buttons & KEY_B ? 200 : origin.button.b);
							break;
					}

					SISetResponse(&status, sizeof(status) * 8);
				}
				break;
			case CMD_ORIGIN:
				if (length == 9) SISetResponse(&origin, sizeof(origin) * 8);
				break;
			case CMD_RECALIBRATE:
			case CMD_STATUS_LONG:
				if (length == 25) {
					id.status.mode  = buffer[1];
					id.status.motor = buffer[2];

					SISetResponse(&origin, sizeof(origin) * 8);
				}
				break;
		}

		set_motor(id.status.motor == MOTOR_RUMBLE);
	}
}
