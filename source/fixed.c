#include <gba_types.h>
#include "fixed.h"

//! Convert an integer to fixed-point
inline s32 int2fx(int d) {
    return d << FIX_SHIFT;
}

//! Convert a fixed point value to an unsigned integer.
inline u32 fx2uint(s32 fx) {
    return fx >> FIX_SHIFT;
}

//! Convert a fixed point value to an signed integer.
inline s32 fx2int(s32 fx) {
    return fx / FIX_SCALE;
}

//! Multiply two fixed point values
inline s32 fxmul(s32 fa, s32 fb) {
    return (fa * fb) >> FIX_SHIFT;
}

//! Divide two fixed point values.
inline s32 fxdiv(s32 fa, s32 fb) {
    return ((fa) * FIX_SCALE) / (fb);
}

