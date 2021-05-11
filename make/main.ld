#define TEXT(fn)    fn(.text*); . = ALIGN(0x10);
#define DATA(fn)    fn(.data*); . = ALIGN(0x10); fn(.rodata*); . = ALIGN(0x10);
#define BSS(fn)     fn(.bss*);  . = ALIGN(0x10);
#define SECTION(addr, name, section)        \
    name##_start = __dev;                   \
    .name addr : AT(__dev) SUBALIGN(0x10)   \
    {section}                               \
    __dev += SIZEOF(.name);                 \
    name##_end = __dev;
#define SECTION_NOLOAD(name, section)       \
    .name.noload (NOLOAD) : SUBALIGN(0x10)  \
    {section}
#define CODE(addr, name, code, bss)                                     \
    code_##name##_start = ADDR(.name);                                  \
    code_##name##_end = ADDR(.name) + SIZEOF(.name);                    \
    bss_##name##_start = ADDR(.name.noload);                            \
    bss_##name##_end = ADDR(.name.noload) + SIZEOF(.name.noload);       \
    bss_##name##_size = SIZEOF(.name.noload);                           \
    SECTION(addr, name, code)                                           \
    SECTION_NOLOAD(name, bss)

#undef mips
OUTPUT_ARCH(mips)
SECTIONS
{
    __dev = 0;
    SECTION(
        0xA4000000, boot,
        TEXT(build/src/header.o)
        TEXT(ultra/lib/PR/ipl3.o)
    )
    CODE(
        0x80000400, main,
        TEXT(ultra/lib/PR/crt0.o)
        TEXT(build/src/main.ld.o)
        TEXT(ultra/lib/PR/rspboot.o)
        TEXT(ultra/lib/PR/gspF3DEX2.fifo.o)
        DATA(build/src/main.ld.o)
        DATA(ultra/lib/PR/rspboot.o)
        DATA(ultra/lib/PR/gspF3DEX2.fifo.o),
        BSS(build/src/main.ld.o)
    )
    bss_main_size_hi = bss_main_size >> 16;
    bss_main_size_lo = bss_main_size & 0xFFFF;
    /DISCARD/ : {*(*);}
}
