#ifndef GBA_FIXED_H
#define GBA_FIXED_H

#define FIX_SCALE           (1 << 8)
#define FIX_SCALEF          (double) FIX_SCALE
#define FIX_SHIFT           8
#define FIX_MASK            0x1FF
#define FIX_HALF            (1 << 7)

inline s32 int2fx(int d);

inline u32 fx2uint(s32 fx);

inline u32 fx2ufrac(s32 fx);

inline s32 fx2int(s32 fx);

inline s32 fxmul(s32 fa, s32 fb);

inline s32 fxdiv(s32 fa, s32 fb);

#endif //GBA_FIXED_H
