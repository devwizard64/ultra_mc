#ifndef _MEM_H_
#define _MEM_H_

#include <ultra64.h>
#include <types.h>

#include "block_gfx.h"

#define HEAP_SIG 0x4D4B

#define mem_alloc(size) mem_alloc_l(size)

struct heap_t
{
    /* 0x00 */  u16     sig;
    /* 0x02 */  u16     free;
    /* 0x04 */  struct heap_t *prev;
    /* 0x08 */  struct heap_t *next;
    /* 0x0C */  size_t  size;
};  /* 0x10 */

extern struct heap_t *mem_heap;

extern void *mem_alloc_gfx(size_t);
extern void *mem_alloc_l(size_t);
extern void *mem_alloc_r(size_t);
extern void  mem_free(void *);
extern void  mem_free_gfx(void *);
extern void  mem_init(void);
extern void  mem_draw(void);
extern void  mem_update_gfx(void);

#endif /* _MEM_H_ */
