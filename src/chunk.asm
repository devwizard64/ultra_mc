/*
void chunk_decompress(u8 *dst, u8 *src)
{
    u8 *end = dst + CHUNK_LEN;
    do
    {
        u8 blk = src[0];
        u8 cnt = src[1];
        do
        {
            *dst = blk;
            cnt--;
            dst++;
        }
        while (cnt != 0);
        src += 2;
    }
    while (dst != end);
}
*/
chunk_decompress:
    addiu   a2, a0, 0x0200
@@loop:
    lbu     v0, 0x0000(a1)
    lbu     v1, 0x0001(a1)
@@fill:
    sb      v0, 0x0000(a0)
    addiu   v1, v1, -0x0001
    bnez    v1, @@fill
    addiu   a0, a0, 0x0001
    bne     a0, a2, @@loop
    addiu   a1, a1, 0x0002
    jr      ra
    nop
