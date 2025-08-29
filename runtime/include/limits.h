#ifndef LIMITS_H
#define LIMITS_H

#define CHAR_BIT 8

#define SCHAR_MAX 127
#define SCHAR_MIN (-128)
#define UCHAR_MAX 255

#if defined(__CHAR_UNSIGNED__) || defined(__CHAR_UNSIGNED)
#define CHAR_MIN 0
#define CHAR_MAX UCHAR_MAX
#else
#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX
#endif

#define MB_LEN_MAX 1

#define SHRT_MAX 32767
#define SHRT_MIN (-32767 - 1)
#define USHRT_MAX 65535u

#define INT_MAX 2147483647
#define INT_MIN (-2147483647 - 1)
#define UINT_MAX 4294967295u

#define LONG_MAX 2147483647L
#define LONG_MIN (-2147483647L - 1)
#define ULONG_MAX 4294967295UL

#define SIZE_MAX UINT_MAX
#define PTRDIFF_MIN INT_MIN
#define PTRDIFF_MAX INT_MAX

#ifdef __WCHAR_MAX__
#define WCHAR_MAX __WCHAR_MAX__
#ifdef __WCHAR_MIN__
#define WCHAR_MIN __WCHAR_MIN__
#else

#if __WCHAR_MAX__ > 0
#define WCHAR_MIN 0
#else
#define WCHAR_MIN (-(__WCHAR_MAX__) - 1)
#endif
#endif
#endif

#endif
