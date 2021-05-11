#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <ultra64.h>
#include <types.h>

extern const u8 texture_printf[];

extern char printf_buf[];

extern void printf(const char *, ...);
extern void printf_draw(void);

#endif /* _PRINTF_H_ */
