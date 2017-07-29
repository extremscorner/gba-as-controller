#ifndef GBA_BIOS_H
#define GBA_BIOS_H

#include <stdint.h>

#define HALT 0x00
#define STOP 0x80

static inline void Halt(void)
{
	asm volatile("mov lr, pc; mov pc, #0x000001A0" ::: "r2", "ip", "lr");
}

static inline void Stop(void)
{
	asm volatile("mov lr, pc; mov pc, #0x000001A8" ::: "r2", "ip", "lr");
}

static inline void CustomHalt(uint8_t flag)
{
	register int r2 asm("r2") = flag;
	asm volatile("mov lr, pc; mov pc, #0x000001AC" :: "r" (r2) : "ip", "lr");
}

static inline void SoundBias(uint32_t bias)
{
	register int r0 asm("r0") = bias;
	asm volatile("svc 0x190000" :: "r" (r0) : "r1", "r2", "r3");
}

#endif /* GBA_BIOS_H */
