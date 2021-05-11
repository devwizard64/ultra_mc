#ifndef _TYPES_H_
#define _TYPES_H_

#include <ultra64.h>

#define false   0
#define true    1

#ifndef __ASSEMBLER__

typedef unsigned int uint;
typedef u8 bool;
typedef s16 vecs[3];
typedef f32 vecf[3];

#define unused                  __attribute__((unused))
#define fallthrough             __attribute__((fallthrough))
#define lenof(x)                (sizeof((x)) / sizeof((x)[0]))

#endif /* __ASSEMBLER__ */

#endif /* _TYPES_H_ */
