#ifndef __PIXMAN_CONF_H__
#define __PIXMAN_CONF_H__

#include <xboot.h>

#if defined(__BIG_ENDIAN)
# define WORDS_BIGENDIAN		(1)
#endif

#if	ULONG_MAX == 0xFFFFFFFFUL
#define SIZEOF_LONG 			(32 / CHAR_BIT)
#elif ULONG_MAX == 0xFFFFFFFFFFFFFFFFUL
#define SIZEOF_LONG 			(64 / CHAR_BIT)
#else
#error "Unsupported size of 'long' type!"
#endif

#ifdef __ARM32__
#if __ARM_ARCH__ >= 6
# define USE_ARM_SIMD
#endif
#ifdef __ARM_IWMMXT__
# define USE_ARM_IWMMXT
#endif
#ifdef __ARM_NEON__
# define USE_ARM_NEON
#endif
#endif

#define PIXMAN_NO_TLS 			(1)

#endif /* __PIXMAN_CONF_H__ */


