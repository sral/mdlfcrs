/* This code is based of the excellent Tonc.
 * Go check it out here: http://www.coranac.com/tonc/text/toc.htm
 */

#ifndef GBA_FIXED_H
#define GBA_FIXED_H

#define FIX_SCALE           (1 << 8)
#define FIX_SHIFT           8

extern s32 int2fx(s32 d);
extern u32 fx2uint(s32 fx);
extern s32 fx2int(s32 fx);
extern s32 fxmul(s32 fa, s32 fb);
extern s32 fxdiv(s32 fa, s32 fb);

#endif //GBA_FIXED_H
