/*
 * libmemfix_i486.so - native fix for Source SDK 2006 srcds on modern CPUs.
 *
 * The corruption ("couldn't exec au", mangled config/command strings) is
 * caused by glibc's CPUID-selected memcpy implementation on modern Intel
 * (and some other) CPUs when used by these old 32-bit binaries.
 *
 * This shim replaces memcpy with a simple portable implementation that does
 * not dispatch on CPUID, so the old engine parses its command line and
 * config files correctly.  It is loaded via LD_PRELOAD from srcds_run.
 *
 * Build:  gcc -m32 -shared -fPIC -O2 -fno-builtin -o libmemfix_i486.so libmemfix_i486.c
 */
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	unsigned char       *d = dst;
	const unsigned char *s = src;
	size_t i = 0;

#if defined(__i386__)
	while (i + 4 <= n) {
		*(uint32_t *)(d + i) = *(const uint32_t *)(s + i);
		i += 4;
	}
#else
	while (i + 8 <= n) {
		*(uint64_t *)(d + i) = *(const uint64_t *)(s + i);
		i += 8;
	}
#endif
	while (i < n) {
		d[i] = s[i];
		i++;
	}

	return dst;
}
