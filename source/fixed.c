/* This code is based of the excellent Tonc.
 * Go check it out here: http://www.coranac.com/tonc/text/toc.htm
 */
#include <gba_types.h>
#include "fixed.h"

// Convert an integer to fixed-point
inline s32 int2fx(s32 d) {
    return d << FIX_SHIFT;
}

// Convert a fixed point value to an unsigned integer.
inline u32 fx2uint(s32 fx) {
    return (u32) fx >> FIX_SHIFT;
}

// Convert a fixed point value to an signed integer.
inline s32 fx2int(s32 fx) {
    return fx >> FIX_SHIFT;
}

// Multiply two fixed point values
inline s32 fxmul(s32 fa, s32 fb) {
    return (fa * fb) >> FIX_SHIFT;
}

// Divide two fixed point values.
inline s32 fxdiv(s32 fa, s32 fb) {
    return ((fa) * FIX_SCALE) / (fb);
}

