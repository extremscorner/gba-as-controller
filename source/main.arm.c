#include <stdint.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_sio.h>

#define ROM            ((uint8_t *)0x08000000)
#define ROM_GPIODATA *((uint16_t *)0x080000C4)
#define ROM_GPIODIR  *((uint16_t *)0x080000C6)
#define ROM_GPIOCNT  *((uint16_t *)0x080000C8)

//#define ANALOG

static uint8_t id[] = {0x29, 0x00, 0x00};

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
} status = {
	.buttons  = { .use_origin = 1 },
	.stick    = { 128, 128 },
};

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
	REG_RCNT = R_GPIO | GPIO_SO_IO | GPIO_SO;
	REG_IE = IRQ_SERIAL;

	while (true) {
		int length = SIGetCommand(buffer, sizeof(buffer) * 8 + 1);
		if (length < 9) continue;

		switch (buffer[0]) {
			case 0x00:
			case 0xFF:
				if (length == 9) {
					if (has_motor()) {
						id[0] = 0x09;
						id[1] = 0x00;
						id[2] = ROM_GPIODATA;
					} else {
						id[0] = 0x29;
						id[1] = 0x00;
						id[2] = 0x00;
					}

					SISetResponse(id, sizeof(id) * 8);
				}
				break;
			case 0x40:
				if (length == 25) {
					unsigned buttons     = ~REG_KEYINPUT;
					status.buttons.a     = !!(buttons & KEY_A);
					status.buttons.b     = !!(buttons & KEY_B);
					status.buttons.z     = !!(buttons & KEY_SELECT);
					status.buttons.start = !!(buttons & KEY_START);
					#ifndef ANALOG
					status.buttons.right = !!(buttons & KEY_RIGHT);
					status.buttons.left  = !!(buttons & KEY_LEFT);
					status.buttons.up    = !!(buttons & KEY_UP);
					status.buttons.down  = !!(buttons & KEY_DOWN);
					#endif
					status.buttons.r     = !!(buttons & KEY_R);
					status.buttons.l     = !!(buttons & KEY_L);

					#ifdef ANALOG
					if (buttons & KEY_RIGHT)
						status.stick.x = origin.stick.x + 100;
					else if (buttons & KEY_LEFT)
						status.stick.x = origin.stick.x - 100;
					else
						status.stick.x = origin.stick.x;

					if (buttons & KEY_UP)
						status.stick.y = origin.stick.y + 100;
					else if (buttons & KEY_DOWN)
						status.stick.y = origin.stick.y - 100;
					else
						status.stick.y = origin.stick.y;
					#endif

					switch (buffer[1]) {
						default:
							status.mode0.substick.x = origin.substick.x;
							status.mode0.substick.y = origin.substick.y;
							status.mode0.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l) >> 4;
							status.mode0.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r) >> 4;
							status.mode0.button.a   = (buttons & KEY_A ? 200 : origin.button.a) >> 4;
							status.mode0.button.b   = (buttons & KEY_B ? 200 : origin.button.b) >> 4;
							break;
						case 0x01:
							status.mode1.substick.x = origin.substick.x >> 4;
							status.mode1.substick.y = origin.substick.y >> 4;
							status.mode1.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l);
							status.mode1.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r);
							status.mode1.button.a   = (buttons & KEY_A ? 200 : origin.button.a) >> 4;
							status.mode1.button.b   = (buttons & KEY_B ? 200 : origin.button.b) >> 4;
							break;
						case 0x02:
							status.mode2.substick.x = origin.substick.x >> 4;
							status.mode2.substick.y = origin.substick.y >> 4;
							status.mode2.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l) >> 4;
							status.mode2.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r) >> 4;
							status.mode2.button.a   = (buttons & KEY_A ? 200 : origin.button.a);
							status.mode2.button.b   = (buttons & KEY_B ? 200 : origin.button.b);
							break;
						case 0x03:
							status.mode3.substick.x = origin.substick.x;
							status.mode3.substick.y = origin.substick.y;
							status.mode3.trigger.l  = (buttons & KEY_L ? 200 : origin.trigger.l);
							status.mode3.trigger.r  = (buttons & KEY_R ? 200 : origin.trigger.r);
							break;
						case 0x04:
							status.mode4.substick.x = origin.substick.x;
							status.mode4.substick.y = origin.substick.y;
							status.mode4.button.a   = (buttons & KEY_A ? 200 : origin.button.a);
							status.mode4.button.b   = (buttons & KEY_B ? 200 : origin.button.b);
							break;
					}

					switch (buffer[2]) {
						default:
							ROM_GPIODATA = 0;
							break;
						case 0x01:
							ROM_GPIOCNT  = 1;
							ROM_GPIODIR  = 1 << 3;
							ROM_GPIODATA = 1 << 3;
							break;
					}

					SISetResponse(&status, sizeof(status) * 8);
				}
				break;
			case 0x41:
				if (length == 9) SISetResponse(&origin, sizeof(origin) * 8);
				break;
			case 0x42:
			case 0x43:
				if (length == 25) SISetResponse(&origin, sizeof(origin) * 8);
				break;
		}
	}
}
