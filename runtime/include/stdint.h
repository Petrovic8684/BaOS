#ifndef STDINT_H
#define STDINT_H

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef int intptr_t;
typedef unsigned int uintptr_t;

#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 0xff

#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 0xffff

#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 0xffffffffU

#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX

#endif
