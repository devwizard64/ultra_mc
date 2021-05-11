#ifndef _FAULT_H_
#define _FAULT_H_

#include <ultra64.h>
#include <types.h>

#define VIDEO_W 320
#define VIDEO_H 240
#define SP_H    12
#define LN_H    9
#define TEXT_X  (VIDEO_W/2 - 3*45)
#define TEXT_Y  20

#ifndef __ASSEMBLER__

extern void fault_darken(u16 *, u16 *);
extern void fault_print(u16 *, const char *);
extern void fault_printf(u16 *, const char *, ...);

extern void fault_main(void *);

#endif /* __ASSEMBLER__ */

#endif /* _FAULT_H_ */
