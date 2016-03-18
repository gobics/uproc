#include "string.h"

static inline void array_reverse(void *p, size_t n, size_t sz)
{
    unsigned char *s = p, tmp;
    size_t i, k, i1, i2;

    for (i = 0; i < n / 2; i++) {
        for (k = 0; k < sz; k++) {
            i1 = sz * i + k;
            i2 = sz * (n - i - 1) + k;
            tmp = s[i1];
            s[i1] = s[i2];
            s[i2] = tmp;
        }
    }
}
#define ARRAY_REVERSE(a) \
    array_reverse((a), sizeof(a) / sizeof *(a), sizeof *(a))

static inline void array_set(void *p, size_t n, size_t sz, const void *v)
{
    for (size_t i = 0; i < n; i++) {
        void *el = p + (i * sz);
        memcpy(el, v, sz);
    }
}

#define ARRAY_SET(a, v) \
    array_set((a), sizeof(a) / sizeof *(a), sizeof *(a), (v))

static inline void array_shiftleft(void *p, size_t n, size_t sz, size_t shift,
                                   const void *pad)
{
    size_t keep = n - shift;
    memmove(p, p + shift * sz, keep * sz);
    array_set(p + keep * sz, shift, sz, pad);
}
