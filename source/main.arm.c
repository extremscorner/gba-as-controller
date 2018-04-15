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

enum {
	CMD_ID = 0x00,
	CMD_STATUS = 0x30,
	CMD_RESET = 0xFF
};

static struct {
	uint8_t type[2];
	uint8_t status;
} id = {{0x08, 0x00}, 0x00};

static struct {
	struct {
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
	} buttons;

	uint8_t flags;
	uint8_t wheel;

	struct { uint8_t l, r; } pedal;
	struct { uint8_t l, r; } paddle;
} status = {
	.buttons = { .use_origin = 1 },
	.wheel   = 128,
};

static uint8_t buffer[128];

void SISetResponse(const void *buf, unsigned bits);
int SIGetCommand(void *buf, unsigned bits);

int IWRAM_CODE main(void)
{
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

		switch (buffer[0]) {
			case CMD_RESET:
				status.wheel = 128;
			case CMD_ID:
				if (length == 9) SISetResponse(&id, sizeof(id) * 8);
				break;
			case CMD_STATUS:
				if (length == 25) {
					unsigned buttons     = ~REG_KEYINPUT;
					status.buttons.a     = !!(buttons & KEY_A);
					status.buttons.b     = !!(buttons & KEY_B);
					status.buttons.z     = !!(buttons & KEY_SELECT);
					status.buttons.start = !!(buttons & KEY_START);
					status.buttons.right = !!(buttons & KEY_RIGHT);
					status.buttons.left  = !!(buttons & KEY_LEFT);
					status.buttons.up    = !!(buttons & KEY_UP);
					status.buttons.down  = !!(buttons & KEY_DOWN);
					status.buttons.r     = !!(buttons & KEY_R);
					status.buttons.l     = !!(buttons & KEY_L);

					SISetResponse(&status, sizeof(status) * 8);
				}
				break;
		}
	}
}
