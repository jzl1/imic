#include <stdio.h>
#include <stdint.h>
#include <string.h>
static inline void cpuid(uint32_t leaf, uint32_t sub, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(sub));
}
int main(void) {
    uint32_t a,b,c,d;
    cpuid(0,0,&a,&b,&c,&d);
    char v[13]; memcpy(v,&b,4); memcpy(v+4,&d,4); memcpy(v+8,&c,4); v[12]=0;
    printf("vendor=%s maxleaf=%u\n", v, a);
    cpuid(1,0,&a,&b,&c,&d);
    printf("leaf1 a=%08x b=%08x c=%08x d=%08x\n", a,b,c,d);
    cpuid(0x80000000,0,&a,&b,&c,&d);
    printf("maxext=%08x\n", a);
    cpuid(0x80000001,0,&a,&b,&c,&d);
    printf("ext c=%08x d=%08x\n", c,d);
    return 0;
}
