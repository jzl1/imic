/*
 * libmemfix_i486.so - native fix for Source SDK 2006 srcds on modern CPUs.
 *
 * The corruption ("couldn't exec au", mangled config/command strings) is
 * caused by glibc's CPUID-selected memcpy implementation on modern Intel
 * (and some other) CPUs when used by these old 32-bit binaries.
 *
 * This shim replaces memcpy with a fixed SSE2 implementation that does not
 * dispatch on CPUID, so the old engine parses its command line and config
 * files correctly.  SSE2 is safe to assume: it is baseline on every x86 CPU
 * made since ~2004 and srcds itself runs the "SSE2 Optimised binary".
 *
 * Build:  gcc -m32 -shared -fPIC -O3 -msse2 -fno-builtin \
 *             -o libmemfix_i486.so libmemfix_i486.c
 */
#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	unsigned char       *d = dst;
	const unsigned char *s = src;

	/* 64-byte block: 4 x 16-byte unaligned loads/stores. */
	while (n >= 64) {
		__m128i v0 = _mm_loadu_si128((const __m128i *)(s + 0));
		__m128i v1 = _mm_loadu_si128((const __m128i *)(s + 16));
		__m128i v2 = _mm_loadu_si128((const __m128i *)(s + 32));
		__m128i v3 = _mm_loadu_si128((const __m128i *)(s + 48));
		_mm_storeu_si128((__m128i *)(d + 0),  v0);
		_mm_storeu_si128((__m128i *)(d + 16), v1);
		_mm_storeu_si128((__m128i *)(d + 32), v2);
		_mm_storeu_si128((__m128i *)(d + 48), v3);
		d += 64;
		s += 64;
		n -= 64;
	}

	while (n >= 16) {
		__m128i v = _mm_loadu_si128((const __m128i *)s);
		_mm_storeu_si128((__m128i *)d, v);
		d += 16;
		s += 16;
		n -= 16;
	}

	if (n >= 8) {
		*(uint64_t *)(void *)d = *(const uint64_t *)(const void *)s;
		d += 8;
		s += 8;
		n -= 8;
	}
	if (n >= 4) {
		*(uint32_t *)(void *)d = *(const uint32_t *)(const void *)s;
		d += 4;
		s += 4;
		n -= 4;
	}
	if (n >= 2) {
		*(uint16_t *)(void *)d = *(const uint16_t *)(const void *)s;
		d += 2;
		s += 2;
		n -= 2;
	}
	if (n == 1) {
		*d = *s;
	}

	return dst;
}

