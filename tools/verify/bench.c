#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    volatile unsigned x = 12345;
    volatile double f = 1.000001;
    char buf[64] = "abcdefghijklmnopqrstuvwxyz0123456789";
    char dst[64];
    double t0 = now_ms();
    for (int i = 0; i < 30000000; i++) {
        x = x * 1103515245u + 12345u;
        x ^= x >> 16;
        f = f * 1.0000001 + 0.000001;
        if ((i & 1023) == 0) {
            f = sqrt(f + i * 1e-9);
            memcpy(dst, buf, 64);
            dst[0] = (char)x;
        }
    }
    double t1 = now_ms();
    printf("done x=%u f=%.9f ms=%.1f\n", x, f, t1 - t0);
    return 0;
}
