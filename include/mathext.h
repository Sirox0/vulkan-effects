#ifndef MATH_EXT_H
#define MATH_EXT_H

#include "numtypes.h"

void clamp(u32* v, u32 min, u32 max);
void clampf(f32* v, f32 min, f32 max);
f32 lerpf(f32 a, f32 b, f32 factor);
u32 gcd(u32 a, u32 b);
u32 lcm(u32 a, u32 b);
f32 randFloat();

#endif