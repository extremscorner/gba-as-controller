#include <stdint.h>
#include <gba_dma.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include "bios.h"

#define ROM           ((int16_t *)0x08000000)
#define ROM_GPIODATA *((int16_t *)0x080000C4)
#define ROM_GPIODIR  *((int16_t *)0x080000C6)
#define ROM_GPIOCNT  *((int16_t *)0x080000C8)

//#define ANALOG

enum {
	CMD_ID = 0x00,
	CMD_STATUS,
	CMD_READ,
	CMD_WRITE,
	CMD_RESET = 0xFF
};

static struct {
	uint8_t type[2];
	uint8_t status;
} id = {{0x05, 0x00}, 0x01};

static struct {
	struct {
		uint16_t right   : 1;
		uint16_t left    : 1;
		uint16_t down    : 1;
		uint16_t up      : 1;
		uint16_t start   : 1;
		uint16_t z       : 1;
		uint16_t b       : 1;
		uint16_t a       : 1;
		uint16_t c_right : 1;
		uint16_t c_left  : 1;
		uint16_t c_down  : 1;
		uint16_t c_up    : 1;
		uint16_t r       : 1;
		uint16_t l       : 1;
		uint16_t         : 1;
		uint16_t reset   : 1;
	} buttons;

	struct { int8_t x, y; } stick;
} status;

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

uint8_t crc8_lut[256] = {
	0x00, 0x85, 0x8F, 0x0A, 0x9B, 0x1E, 0x14, 0x91, 0xB3, 0x36, 0x3C, 0xB9, 0x28, 0xAD, 0xA7, 0x22,
	0xE3, 0x66, 0x6C, 0xE9, 0x78, 0xFD, 0xF7, 0x72, 0x50, 0xD5, 0xDF, 0x5A, 0xCB, 0x4E, 0x44, 0xC1,
	0x43, 0xC6, 0xCC, 0x49, 0xD8, 0x5D, 0x57, 0xD2, 0xF0, 0x75, 0x7F, 0xFA, 0x6B, 0xEE, 0xE4, 0x61,
	0xA0, 0x25, 0x2F, 0xAA, 0x3B, 0xBE, 0xB4, 0x31, 0x13, 0x96, 0x9C, 0x19, 0x88, 0x0D, 0x07, 0x82,
	0x86, 0x03, 0x09, 0x8C, 0x1D, 0x98, 0x92, 0x17, 0x35, 0xB0, 0xBA, 0x3F, 0xAE, 0x2B, 0x21, 0xA4,
	0x65, 0xE0, 0xEA, 0x6F, 0xFE, 0x7B, 0x71, 0xF4, 0xD6, 0x53, 0x59, 0xDC, 0x4D, 0xC8, 0xC2, 0x47,
	0xC5, 0x40, 0x4A, 0xCF, 0x5E, 0xDB, 0xD1, 0x54, 0x76, 0xF3, 0xF9, 0x7C, 0xED, 0x68, 0x62, 0xE7,
	0x26, 0xA3, 0xA9, 0x2C, 0xBD, 0x38, 0x32, 0xB7, 0x95, 0x10, 0x1A, 0x9F, 0x0E, 0x8B, 0x81, 0x04,
	0x89, 0x0C, 0x06, 0x83, 0x12, 0x97, 0x9D, 0x18, 0x3A, 0xBF, 0xB5, 0x30, 0xA1, 0x24, 0x2E, 0xAB,
	0x6A, 0xEF, 0xE5, 0x60, 0xF1, 0x74, 0x7E, 0xFB, 0xD9, 0x5C, 0x56, 0xD3, 0x42, 0xC7, 0xCD, 0x48,
	0xCA, 0x4F, 0x45, 0xC0, 0x51, 0xD4, 0xDE, 0x5B, 0x79, 0xFC, 0xF6, 0x73, 0xE2, 0x67, 0x6D, 0xE8,
	0x29, 0xAC, 0xA6, 0x23, 0xB2, 0x37, 0x3D, 0xB8, 0x9A, 0x1F, 0x15, 0x90, 0x01, 0x84, 0x8E, 0x0B,
	0x0F, 0x8A, 0x80, 0x05, 0x94, 0x11, 0x1B, 0x9E, 0xBC, 0x39, 0x33, 0xB6, 0x27, 0xA2, 0xA8, 0x2D,
	0xEC, 0x69, 0x63, 0xE6, 0x77, 0xF2, 0xF8, 0x7D, 0x5F, 0xDA, 0xD0, 0x55, 0xC4, 0x41, 0x4B, 0xCE,
	0x4C, 0xC9, 0xC3, 0x46, 0xD7, 0x52, 0x58, 0xDD, 0xFF, 0x7A, 0x70, 0xF5, 0x64, 0xE1, 0xEB, 0x6E,
	0xAF, 0x2A, 0x20, 0xA5, 0x34, 0xB1, 0xBB, 0x3E, 0x1C, 0x99, 0x93, 0x16, 0x87, 0x02, 0x08, 0x8D,
};

static uint8_t pak_copyto(uint16_t addr, uint8_t *src)
{
	uint16_t *dst = (uint16_t *)VRAM + addr;
	uint8_t crc = 0;

	for (int i = 0; i < 32; i++) {
		crc ^= *dst++ = *src++;
		crc  = crc8_lut[crc];
	}

	return crc;
}

static uint8_t pak_copyfrom(uint16_t addr, uint8_t *dst, uint8_t mask)
{
	uint16_t *src = (uint16_t *)VRAM + addr;
	uint8_t crc = 0;

	for (int i = 0; i < 32; i++) {
		crc ^= *dst++ = *src++ & mask;
		crc  = crc8_lut[crc];
	}

	return crc;
}

uint8_t crc5_lut[32] = {
	0x00, 0x15, 0x1F, 0x0A, 0x0B, 0x1E, 0x14, 0x01,
	0x16, 0x03, 0x09, 0x1C, 0x1D, 0x08, 0x02, 0x17,
	0x19, 0x0C, 0x06, 0x13, 0x12, 0x07, 0x0D, 0x18,
	0x0F, 0x1A, 0x10, 0x05, 0x04, 0x11, 0x1B, 0x0E,
};

static uint8_t crc5(uint16_t addr)
{
	uint8_t crc = addr & 0x8000 ? 0x15 : 0;

	crc ^= (addr >> 10) & 0x1F;
	crc  = crc5_lut[crc];
	crc ^= (addr >> 5) & 0x1F;
	crc  = crc5_lut[crc];

	return crc;
}

void SISetResponse(const void *buf, unsigned bits);
int SIGetCommand(void *buf, unsigned bits);

int IWRAM_CODE main(void)
{
	RegisterRamReset(RESET_ALL_REG);

	REG_IE = IRQ_SERIAL | IRQ_TIMER1 | IRQ_TIMER0;
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
			case CMD_ID:
				if (length == 9) SISetResponse(&id, sizeof(id) * 8);
				break;
			case CMD_STATUS:
				if (length == 9) {
					unsigned buttons     = ~REG_KEYINPUT;
					#ifndef ANALOG
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
					#else
					status.buttons.a     = !!(buttons & KEY_A);
					status.buttons.b     = !!(buttons & KEY_B);
					status.buttons.l     = !!(buttons & KEY_SELECT);
					status.buttons.start = !!(buttons & KEY_START);
					status.buttons.r     = !!(buttons & KEY_R);
					status.buttons.z     = !!(buttons & KEY_L);

					if (buttons & KEY_RIGHT)
						status.stick.x = +80;
					else if (buttons & KEY_LEFT)
						status.stick.x = -80;
					else
						status.stick.x = 0;

					if (buttons & KEY_UP)
						status.stick.y = +80;
					else if (buttons & KEY_DOWN)
						status.stick.y = -80;
					else
						status.stick.y = 0;
					#endif

					SISetResponse(&status, sizeof(status) * 8);
				}
				break;
			case CMD_READ:
				if (length == 25) {
					uint16_t address   = (buffer[2] | buffer[1] << 8) & ~0x1F;
					if (crc5(address) != (buffer[2] & 0x1F)) break;

					buffer[35] = pak_copyfrom(address, &buffer[3],
						(address & 0x8000) == 0x8000 && rumble ? 0x81 : 0xFF);

					SISetResponse(&buffer[3], 264);
				}
				break;
			case CMD_WRITE:
				if (length == 281) {
					uint16_t address   = (buffer[2] | buffer[1] << 8) & ~0x1F;
					if (crc5(address) != (buffer[2] & 0x1F)) break;

					buffer[35] = pak_copyto(address, &buffer[3]);

					SISetResponse(&buffer[35], 8);

					if ((address & 0x8000) == 0x8000 && has_motor())
						set_motor(buffer[3] & 0x01);
				}
				break;
		}
	}
}
