/*
 * libmemfix_variants.c - memcpy shim built with different ISA flags.
 * Same idea as libmemfix_i486.c but vector width selected at compile time:
 *   -DVEC16 -> 16-byte SSE2/SSSE3/SSE4.1
 *   -DVEC32 -> 32-byte AVX/AVX2
 */
#include <stddef.h>
#include <stdint.h>
#ifdef VEC32
#include <immintrin.h>
#define BLOCK 32
#else
#include <emmintrin.h>
#define BLOCK 16
#endif

void *memcpy(void *dst, const void *src, size_t n)
{
	unsigned char       *d = dst;
	const unsigned char *s = src;

#ifdef VEC32
	/* 128-byte block: 4 x 32-byte unaligned loads/stores. */
	while (n >= 128) {
		__m256i v0 = _mm256_loadu_si256((const __m256i *)(s + 0));
		__m256i v1 = _mm256_loadu_si256((const __m256i *)(s + 32));
		__m256i v2 = _mm256_loadu_si256((const __m256i *)(s + 64));
		__m256i v3 = _mm256_loadu_si256((const __m256i *)(s + 96));
		_mm256_storeu_si256((__m256i *)(d + 0),  v0);
		_mm256_storeu_si256((__m256i *)(d + 32), v1);
		_mm256_storeu_si256((__m256i *)(d + 64), v2);
		_mm256_storeu_si256((__m256i *)(d + 96), v3);
		d += 128;
		s += 128;
		n -= 128;
	}
	while (n >= 32) {
		__m256i v = _mm256_loadu_si256((const __m256i *)s);
		_mm256_storeu_si256((__m256i *)d, v);
		d += 32;
		s += 32;
		n -= 32;
	}
	/* 16-byte tail block (handles 16..31 bytes). */
	while (n >= 16) {
		__m128i v = _mm_loadu_si128((const __m128i *)s);
		_mm_storeu_si128((__m128i *)d, v);
		d += 16;
		s += 16;
		n -= 16;
	}
#else
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
#endif

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
