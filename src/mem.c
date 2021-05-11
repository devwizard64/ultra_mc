#include <ultra64.h>

#include "video.h"
#include "mem.h"
#include "printf.h"

struct heap_t *mem_heap;
static void *mem_free_table[0x400*BLOCK_MTL_LEN];
static uint  mem_free_index;

extern u8 bss_main_end[];

void *mem_alloc_gfx(size_t size)
{
    u8 *addr = video_mem - ((size+0x07) & ~0x07);
    if (addr >= (u8 *)video_gfx)
    {
        video_mem = addr;
        return addr;
    }
    return NULL;
}

void *mem_alloc_l(size_t size)
{
    size_t h_size;
    struct heap_t *heap;
    size = (size+0x07) & ~0x07;
    h_size = size + sizeof(struct heap_t);
    heap = mem_heap;
    while (heap != NULL)
    {
        if (heap->sig == HEAP_SIG)
        {
            if (heap->free && heap->size >= h_size)
            {
                struct heap_t *next = (struct heap_t *)((u8 *)(heap+1) + size);
                next->prev = heap;
                next->next = heap->next;
                next->size = heap->size - h_size;
                next->sig  = HEAP_SIG;
                next->free = true;
                heap->next = next;
                heap->size = size;
                heap->free = false;
                return heap+1;
            }
        }
        else
        {
            printf("M.AL: badsig %08X\n", (u32)heap);
        }
        heap = heap->next;
    }
    return NULL;
}

void *mem_alloc_r(size_t size)
{
    size_t h_size;
    struct heap_t *heap;
    size = (size+0x07) & ~0x07;
    h_size = size + sizeof(struct heap_t);
    heap = mem_heap;
    while (heap != NULL)
    {
        if (heap->sig == HEAP_SIG)
        {
            if (heap->free && heap->size >= h_size)
            {
                struct heap_t *next;
                heap->size -= h_size;
                next = (struct heap_t *)((u8 *)(heap+1) + heap->size);
                next->prev = heap;
                next->next = heap->next;
                next->size = size;
                next->sig  = HEAP_SIG;
                next->free = false;
                heap->next = next;
                return next+1;
            }
        }
        else
        {
            printf("M.AR: badsig %08X\n", (u32)heap);
        }
        heap = heap->next;
    }
    return NULL;
}

void mem_free(void *ptr)
{
    struct heap_t *heap = ((struct heap_t *)ptr) - 1;
    if (heap->sig == HEAP_SIG)
    {
        struct heap_t *next;
        struct heap_t *prev;
        heap->free = true;
        next = (struct heap_t *)((u8 *)(heap+1) + heap->size);
        if (heap->next == next && next->free)
        {
            heap->next = next->next;
            heap->size += sizeof(*next) + next->size;
        }
        prev = heap->prev;
        if (prev != NULL)
        {
            next = (struct heap_t *)((u8 *)(prev+1) + prev->size);
            if (heap == next && prev->free)
            {
                prev->next = heap->next;
                prev->size += sizeof(*heap) + heap->size;
            }
        }
    }
    else
    {
        printf("M.FR: badsig %08X\n", (u32)heap);
    }
}

static void mem_link(void *start, void *end)
{
    struct heap_t **next = &mem_heap;
    struct heap_t *prev;
    struct heap_t *heap;
    while (true)
    {
        prev = *next;
        if (prev == NULL)
        {
            break;
        }
        next = &prev->next;
    }
    *next = heap = start;
    heap->prev = prev;
    heap->next = NULL;
    heap->size = (u8 *)end-(u8 *)start - sizeof(*heap);
    heap->sig  = HEAP_SIG;
    heap->free = true;
}

void mem_free_gfx(void *ptr)
{
    mem_free_table[mem_free_index++] = ptr;
}

void mem_init(void)
{
    mem_heap = NULL;
    mem_free_index = 0;
    mem_link(bss_main_end, (u8 *)0x80000000+osMemSize);
}

void mem_draw(void)
{
    size_t used = 0;
    size_t free = 0;
    struct heap_t *heap = mem_heap;
    while (heap != NULL)
    {
        if (heap->sig == HEAP_SIG)
        {
            used += sizeof(*heap);
            if (heap->free)
            {
                free += heap->size;
            }
            else
            {
                used += heap->size;
            }
        }
        heap = heap->next;
    }
    printf("hu:%4dK  hf:%4dK\n", used/1024, free/1024);
}

void mem_update_gfx(void)
{
    uint i = mem_free_index;
    mem_free_index = 0;
    while (i > 0)
    {
        i--;
        mem_free(mem_free_table[i]);
        mem_free_table[i] = NULL;
    }
}
