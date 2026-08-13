#include <stddef.h>
void *memcpy(void *d,const void *s,size_t n){unsigned char *x=d;const unsigned char *y=s;for(size_t i=0;i<n;i++)x[i]=y[i];return d;} void *memmove(void *d,const void *s,size_t n){unsigned char *x=d;const unsigned char *y=s;if(x<y)for(size_t i=0;i<n;i++)x[i]=y[i];else for(size_t i=n;i>0;i--)x[i-1]=y[i-1];return d;}
