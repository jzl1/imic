#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}
void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    if (d < s) for (size_t i = 0; i < n; i++) d[i] = s[i];
    else for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    return dst;
}
void *memset(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; i++) d[i] = (unsigned char)c;
    return dst;
}
int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < n; i++) if (x[i] != y[i]) return x[i] - y[i];
    return 0;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
char *strcpy(char *d, const char *s) { char *o = d; while ((*d++ = *s++)); return o; }
char *strncpy(char *d, const char *s, size_t n) { char *o = d; size_t i = 0; for (; i < n && s[i]; i++) d[i] = s[i]; for (; i < n; i++) d[i] = 0; return o; }
char *strcat(char *d, const char *s) { char *o = d; while (*d) d++; while ((*d++ = *s++)); return o; }
char *strncat(char *d, const char *s, size_t n) { char *o = d; while (*d) d++; size_t i = 0; for (; i < n && s[i]; i++) d[i] = s[i]; d[i] = 0; return o; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }
int strncmp(const char *a, const char *b, size_t n) { for (size_t i = 0; i < n; i++) { if (a[i] != b[i] || !a[i]) return (unsigned char)a[i] - (unsigned char)b[i]; } return 0; }
int strcasecmp(const char *a, const char *b) { static const char lo[256] = {['A']='a',['B']='b',['C']='c',['D']='d',['E']='e',['F']='f',['G']='g',['H']='h',['I']='i',['J']='j',['K']='k',['L']='l',['M']='m',['N']='n',['O']='o',['P']='p',['Q']='q',['R']='r',['S']='s',['T']='t',['U']='u',['V']='v',['W']='w',['X']='x',['Y']='y',['Z']='z'}; while (*a && lo[(unsigned char)*a] == lo[(unsigned char)*b]) { a++; b++; } return lo[(unsigned char)*a] - lo[(unsigned char)*b]; }
int strncasecmp(const char *a, const char *b, size_t n) { static const char lo[256] = {['A']='a',['B']='b',['C']='c',['D']='d',['E']='e',['F']='f',['G']='g',['H']='h',['I']='i',['J']='j',['K']='k',['L']='l',['M']='m',['N']='n',['O']='o',['P']='p',['Q']='q',['R']='r',['S']='s',['T']='t',['U']='u',['V']='v',['W']='w',['X']='x',['Y']='y',['Z']='z'}; for (size_t i = 0; i < n; i++) { unsigned char x = lo[(unsigned char)a[i]], y = lo[(unsigned char)b[i]]; if (x != y || !a[i]) return x - y; } return 0; }
char *strchr(const char *s, int c) { for (; *s; s++) if ((unsigned char)*s == (unsigned char)c) return (char*)s; return ((unsigned char)c == 0) ? (char*)s : NULL; }
char *strstr(const char *h, const char *n) { if (!*n) return (char*)h; for (; *h; h++) { const char *a = h, *b = n; while (*a && *a == *b) { a++; b++; } if (!*b) return (char*)h; } return NULL; }
char *strrchr(const char *s, int c) { const char *last = NULL; for (; *s; s++) if ((unsigned char)*s == (unsigned char)c) last = s; if ((unsigned char)c == 0) last = s; return (char*)last; }
size_t strspn(const char *s, const char *a) { size_t n = 0; while (s[n]) { const char *p = a; while (*p && *p != s[n]) p++; if (!*p) break; n++; } return n; }
size_t strcspn(const char *s, const char *a) { size_t n = 0; while (s[n]) { const char *p = a; while (*p && *p != s[n]) p++; if (*p) break; n++; } return n; }
char *strpbrk(const char *s, const char *a) { for (; *s; s++) { const char *p = a; while (*p && *p != *s) p++; if (*p) return (char*)s; } return NULL; }
char *strtok(char *s, const char *delim) { static char *save = NULL; if (!s) s = save; if (!s) return NULL; s += strspn(s, delim); if (!*s) { save = NULL; return NULL; } char *end = s + strcspn(s, delim); if (*end) { *end++ = 0; save = end; } else { save = NULL; } return s; }
