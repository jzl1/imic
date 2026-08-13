#include <stddef.h>
void *memset(void *d,int c,size_t n){unsigned char *x=d;for(size_t i=0;i<n;i++)x[i]=(unsigned char)c;return d;}
