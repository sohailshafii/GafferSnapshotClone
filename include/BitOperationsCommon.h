#pragma once

#define BITS_PER_BYTE 8
#define BYTES_PER_WORD 4
#define BITS_PER_WORD 32

#define CPU_LITTLE_ENDIAN 1
#define CPU_BIG_ENDIAN 2

// Use compiler-provided byte order macros rather than maintaining a list of
// architecture defines, which is fragile — e.g. __arm__ misses ARM64/Apple
// Silicon (__aarch64__). __BYTE_ORDER__ is defined by GCC and Clang on all
// supported platforms.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define CPU_ENDIAN CPU_LITTLE_ENDIAN
#else
  #define CPU_ENDIAN CPU_BIG_ENDIAN
#endif

inline uint32_t endianness_fix( uint32_t value )
{
#if CPU_ENDIAN == CPU_BIG_ENDIAN
    return __builtin_bswap32( value );
#else
    return value;
#endif
}


inline int bits_required(int32_t min, int32_t max) {
    // counts number of leading 0s, then subtracts from 32 to get
    // bits required
    return 32 - __builtin_clz(max - min);
}
