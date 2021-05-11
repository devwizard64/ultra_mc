#include <ultra64.h>

static u8 rng_seed_a = 0;
static u8 rng_seed_b = 0;

static u8 rng_u8(void)
{
    register u8 sb;
    rng_seed_a = 5*rng_seed_a + 1;
    sb = rng_seed_b;
    rng_seed_b <<= 1;
    if (sb >> 7 == (sb >> 4 & 1))
    {
        rng_seed_b++;
    }
    return rng_seed_a ^ rng_seed_b;
}

u16 rng_u16(void)
{
    return rng_u8() << 8 | rng_u8();
}

f32 sin(s16 x)
{
    return sinf((f32)(M_PI/0x8000) * x);
}

f32 cos(s16 x)
{
    return cosf((f32)(M_PI/0x8000) * x);
}

void mtxf_mul(f32 mf[4][4], f32 a[4][4], f32 b[4][4])
{
    f32 dst[4][4];
    u32 y;
    for (y = 0; y < 4; y++)
    {
        u32 x;
        for (x = 0; x < 4; x++)
        {
            dst[y][x] =
                a[y][0]*b[0][x] + a[y][1]*b[1][x] +
                a[y][2]*b[2][x] + a[y][3]*b[3][x];
        }
    }
    memcpy(mf, dst, sizeof(dst));
}

void mtxf_tr_zyx(f32 mf[4][4], f32 tx, f32 ty, f32 tz, s16 rx, s16 ry, s16 rz)
{
    register f32 sx = sin(rx);
    register f32 cx = cos(rx);
    register f32 sy = sin(ry);
    register f32 cy = cos(ry);
    register f32 sz = sin(rz);
    register f32 cz = cos(rz);
    mf[0][0] =  cy*cz;
    mf[0][1] =  cy*sz;
    mf[0][2] = -sy;
    mf[0][3] = 0.0F;
    mf[1][0] =  sx*sy*cz - cx*sz;
    mf[1][1] =  sx*sy*sz + cx*cz;
    mf[1][2] =  sx*cy;
    mf[1][3] = 0.0F;
    mf[2][0] =  cx*sy*cz + sx*sz;
    mf[2][1] =  cx*sy*sz - sx*cz;
    mf[2][2] =  cx*cy;
    mf[2][3] = 0.0F;
    mf[3][0] = tx;
    mf[3][1] = ty;
    mf[3][2] = tz;
    mf[3][3] = 1.0F;
}

#if 0
void mtxf_tr_yxz(f32 mf[4][4], f32 tx, f32 ty, f32 tz, s16 rx, s16 ry, s16 rz)
{
    register f32 sx = sin(rx);
    register f32 cx = cos(rx);
    register f32 sy = sin(ry);
    register f32 cy = cos(ry);
    register f32 sz = sin(rz);
    register f32 cz = cos(rz);
    mf[0][0] =  cy*cz + sx*sy*sz;
    mf[0][1] =  cx*sz;
    mf[0][2] = -sy*cz + sx*cy*sz;
    mf[0][3] = 0.0F;
    mf[1][0] = -cy*sz + sx*sy*cz;
    mf[1][1] =  cx*cz;
    mf[1][2] =  sy*sz + sx*cy*cz;
    mf[1][3] = 0.0F;
    mf[2][0] =  cx*sy;
    mf[2][1] = -sx;
    mf[2][2] =  cx*cy;
    mf[2][3] = 0.0F;
    mf[3][0] = tx;
    mf[3][1] = ty;
    mf[3][2] = tz;
    mf[3][3] = 1.0F;
}
#endif
