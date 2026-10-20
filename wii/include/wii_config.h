/**
 * wii_config.h - Compiler compatibility layer for compiling The Wind Waker decompilation
 * with devkitPPC (PowerPC GCC) on the Nintendo Wii.
 */

#ifndef WII_CONFIG_H
#define WII_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <cmath>
#include <math.h>
#include <cctype>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <memory>
#include <algorithm>
#include <locale>

// Undefine newlib ctype macro _C so it does not collide with member names like `s16 _C;` in f_op_msg_mng.h
#undef _C

namespace std {
using ::acosf;
using ::asinf;
using ::atan2f;
using ::atanf;
using ::ceilf;
using ::cosf;
using ::expf;
using ::fabsf;
using ::floorf;
using ::fmodf;
using ::logf;
using ::powf;
using ::sinf;
using ::sqrtf;
using ::tanf;
}

namespace std {
struct __tag_va_List {
    va_list list;
};
}
#else
#include <ctype.h>
#undef _C
#endif // __cplusplus

// PowerPC hardware intrinsics for devkitPPC GCC
#ifdef __GNUC__

static inline int __cntlzw(unsigned int val) {
    int res;
    __asm__ ("cntlzw %0, %1" : "=r"(res) : "r"(val));
    return res;
}

static inline void __dcbz(void* addr, int offset) {
    __asm__ volatile ("dcbz %0, %1" : : "r"(offset), "r"(addr) : "memory");
}

static inline void __sync(void) {
    __asm__ volatile ("sync" : : : "memory");
}

static inline float __fres(float val) {
    float res;
    __asm__ ("fres %0, %1" : "=f"(res) : "f"(val));
    return res;
}

static inline double __frsqrte(double val) {
    double res;
    __asm__ ("frsqrte %0, %1" : "=f"(res) : "f"(val));
    return res;
}

static inline int __rlwimi(int ra, int rs, int sh, int mb, int me) {
    uint32_t s = (uint32_t)rs;
    uint32_t n = (uint32_t)sh & 31u;
    uint32_t rot = n == 0 ? s : ((s << n) | (s >> (32u - n)));
    uint32_t from_mb = 0xFFFFFFFFu >> ((uint32_t)mb & 31u);
    uint32_t to_me = 0xFFFFFFFFu << (31u - ((uint32_t)me & 31u));
    uint32_t mask = ((uint32_t)mb & 31u) <= ((uint32_t)me & 31u) ? (from_mb & to_me)
                                                                 : (from_mb | to_me);
    return (int)((rot & mask) | ((uint32_t)ra & ~mask));
}

#endif // __GNUC__

#define COS_MSL_M_PI 3.14159265358979323846f
#ifndef DEG_TO_RAD
#define DEG_TO_RAD(degrees) (degrees * (COS_MSL_M_PI / 180.0f))
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG(radians) (radians * (180.0f / COS_MSL_M_PI + 0.000005f))
#endif

// Display list helpers
#ifndef LOAD_BP_REG
#define U32_AS_U8(v) (((v) >> 24) & 0xFF), (((v) >> 16) & 0xFF), (((v) >> 8) & 0xFF), (((v) >> 0) & 0xFF)
#define U24_AS_U8(v) (((v) >> 16) & 0xFF), (((v) >> 8) & 0xFF), (((v) >> 0) & 0xFF)
#define U16_AS_U8(v) (((v) >> 8) & 0xFF), (((v) >> 0) & 0xFF)
#define IMAGE_ADDR(addr) (u32)(uintptr_t)(addr) >> 5
#define LOAD_BP_REG(reg, value) GX_CMD_LOAD_BP_REG, reg, U24_AS_U8(value)
#define LOAD_XF_REG(reg, num_args, ...) GX_CMD_LOAD_XF_REG, U16_AS_U8(num_args-1), U16_AS_U8(reg), __VA_ARGS__
#define LOAD_CP_REG(reg, value) GX_CMD_LOAD_CP_REG, reg, U32_AS_U8(value)
#endif

#endif // WII_CONFIG_H
