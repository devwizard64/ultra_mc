#ifndef _MATH_H_
#define _MATH_H_

#include <ultra64.h>
#include <types.h>

extern u16 rng_u16(void);
extern f32 sin(s16);
extern f32 cos(s16);
extern void mtxf_mul(f32[4][4], f32[4][4], f32[4][4]);
extern void mtxf_tr_zyx(f32[4][4], f32, f32, f32, s16, s16, s16);

#endif /* _MATH_H_ */
