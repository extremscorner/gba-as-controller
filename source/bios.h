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

#ifndef GBA_BIOS_H
#define GBA_BIOS_H

#include <stdint.h>

#define RESET_EWRAM     0x01
#define RESET_IWRAM     0x02
#define RESET_PALETTE   0x04
#define RESET_VRAM      0x08
#define RESET_OAM       0x10
#define RESET_ALL_RAM   0x1F
#define RESET_SIO_REG   0x20
#define RESET_SOUND_REG 0x40
#define RESET_REG       0x80
#define RESET_ALL_REG   0xE0
#define RESET_ALL       0xFF

#define HALT 0x00
#define STOP 0x80

struct Div {
	int32_t quot;
	int32_t rem;
	uint32_t quot_abs;
};

struct BitUnPack {
	uint16_t size;
	uint8_t in_bits;
	uint8_t out_bits;
	uint32_t bias      : 31;
	uint32_t bias_zero :  1;
};

#if defined(__thumb__)

static inline void RegisterRamReset(uint8_t flag)
{
	register int r0 asm("r0") = flag;
	asm volatile("svc 0x01" : "+r" (r0) :: "r1", "r2", "r3", "memory");
}

static inline void Halt(void)
{
	asm volatile("svc 0x02" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline void Stop(void)
{
	asm volatile("svc 0x03" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline void VBlankIntrWait(void)
{
	asm volatile("svc 0x05" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline struct Div Div(int32_t num, int32_t denom)
{
	register int r0 asm("r0") = num;
	register int r1 asm("r1") = denom;
	register int r3 asm("r3");
	asm volatile("svc 0x06" : "+r" (r0), "+r" (r1), "=r" (r3) :: "r2", "memory");
	return (struct Div){r0, r1, r3};
}

static inline void BitUnPack(const void *src, void *dst, struct BitUnPack *bup)
{
	register int r0 asm("r0") = (int)src;
	register int r1 asm("r1") = (int)dst;
	register int r2 asm("r2") = (int)bup;
	asm volatile("svc 0x10" : "+r" (r2) : "r" (r0), "r" (r1) : "r3", "memory");
}

static inline void LZ77UnCompVram(const void *src, void *dst)
{
	register int r0 asm("r0") = (int)src;
	register int r1 asm("r1") = (int)dst;
	asm volatile("svc 0x12" :: "r" (r0), "r" (r1) : "r2", "r3", "memory");
}

static inline void SoundBias(uint32_t bias)
{
	register int r0 asm("r0") = bias;
	asm volatile("svc 0x19" :: "r" (r0) : "r1", "r2", "r3", "memory");
}

static inline void HardReset(void)
{
	asm volatile("svc 0x26" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline void CustomHalt(uint8_t flag)
{
	register int r2 asm("r2") = flag;
	asm volatile("svc 0x27" :: "r" (r2) : "r0", "r1", "r3", "memory");
}

#else

static inline void RegisterRamReset(uint8_t flag)
{
	register int r0 asm("r0") = flag;
	asm volatile("svc 0x010000" : "+r" (r0) :: "r1", "r2", "r3", "memory");
}

static inline void Halt(void)
{
	asm volatile("mov lr, pc; mov pc, #0x000001A0" ::: "r2", "ip", "lr", "memory");
}

static inline void Stop(void)
{
	asm volatile("mov lr, pc; mov pc, #0x000001A8" ::: "r2", "ip", "lr", "memory");
}

static inline void VBlankIntrWait(void)
{
	asm volatile("svc 0x050000" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline struct Div Div(int32_t num, int32_t denom)
{
	register int r0 asm("r0") = num;
	register int r1 asm("r1") = denom;
	register int r3 asm("r3");
	asm volatile("svc 0x060000" : "+r" (r0), "+r" (r1), "=r" (r3) :: "r2", "memory");
	return (struct Div){r0, r1, r3};
}

static inline void BitUnPack(const void *src, void *dst, struct BitUnPack *bup)
{
	register int r0 asm("r0") = (int)src;
	register int r1 asm("r1") = (int)dst;
	register int r2 asm("r2") = (int)bup;
	asm volatile("svc 0x100000" : "+r" (r2) : "r" (r0), "r" (r1) : "r3", "memory");
}

static inline void LZ77UnCompVram(const void *src, void *dst)
{
	register int r0 asm("r0") = (int)src;
	register int r1 asm("r1") = (int)dst;
	asm volatile("svc 0x120000" :: "r" (r0), "r" (r1) : "r2", "r3", "memory");
}

static inline void SoundBias(uint32_t bias)
{
	register int r0 asm("r0") = bias;
	asm volatile("svc 0x190000" :: "r" (r0) : "r1", "r2", "r3", "memory");
}

static inline void HardReset(void)
{
	asm volatile("svc 0x260000" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline void CustomHalt(uint8_t flag)
{
	register int r2 asm("r2") = flag;
	asm volatile("mov lr, pc; mov pc, #0x000001AC" :: "r" (r2) : "ip", "lr", "memory");
}

#endif

#endif /* GBA_BIOS_H */
