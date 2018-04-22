#include <stdint.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include "bios.h"

#define ROM            ((uint8_t *)0x08000000)
#define ROM_GPIODATA *((uint16_t *)0x080000C4)
#define ROM_GPIODIR  *((uint16_t *)0x080000C6)
#define ROM_GPIOCNT  *((uint16_t *)0x080000C8)

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
	uint16_t a          : 1;
	uint16_t b          : 1;
	uint16_t x          : 1;
	uint16_t y          : 1;
	uint16_t start      : 1;
	uint16_t get_origin : 1;
	uint16_t err_latch  : 1;
	uint16_t err_status : 1;
	uint16_t left       : 1;
	uint16_t right      : 1;
	uint16_t down       : 1;
	uint16_t up         : 1;
	uint16_t z          : 1;
	uint16_t r          : 1;
	uint16_t l          : 1;
	uint16_t use_origin : 1;
};

static struct {
	uint8_t type[2];

	struct {
		uint8_t mode   : 3;
		uint8_t motor  : 2;
		uint8_t origin : 1;
		uint8_t        : 2;
	} status;
} id;

static struct {
	struct buttons buttons;
	struct { uint8_t x, y; } stick;
	union {
		struct {
			struct { uint8_t x : 8, y : 8; } substick;
			struct { uint8_t r : 4, l : 4; } trigger;
			struct { uint8_t b : 4, a : 4; } button;
		} mode0;

		struct {
			struct { uint8_t y : 4, x : 4; } substick;
			struct { uint8_t l : 8, r : 8; } trigger;
			struct { uint8_t b : 4, a : 4; } button;
		} mode1;

		struct {
			struct { uint8_t y : 4, x : 4; } substick;
			struct { uint8_t r : 4, l : 4; } trigger;
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

static bool has_motor(void)
{
	if (0x96 == ROM[0xB2]) {
		switch (ROM[0xAC]) {
			case 'R':
			case 'V':
				return true;
			default:
				return false;
		}
	}

	return false;
}

void SISetResponse(const void *buf, unsigned bits);
int SIGetCommand(void *buf, unsigned bits);

int IWRAM_CODE main(void)
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

		switch (buffer[0]) {
			case CMD_RESET:
				id.status.motor = MOTOR_STOP;
			case CMD_ID:
				if (length == 9) {
					if (has_motor()) {
						id.type[0] = 0x09;
						id.type[1] = 0x00;
					} else {
						id.type[0] = 0x29;
						id.type[1] = 0x00;
					}

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

		switch (id.status.motor) {
			default:
				ROM_GPIODATA = 0;
				break;
			case MOTOR_RUMBLE:
				ROM_GPIOCNT  = 1;
				ROM_GPIODIR  = 1 << 3;
				ROM_GPIODATA = 1 << 3;
				break;
		}
	}
}
