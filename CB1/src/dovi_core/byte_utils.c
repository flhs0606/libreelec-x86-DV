/* Bytewise comparison, copying and clearing for packed control data. */
#include "byte_utils.h"

typedef char byte_utils_int32[(sizeof(int) == 4) ? 1 : -1];

void mem_cpy_byte(void *destination, const void *source, int length)
{
    unsigned char *dst = (unsigned char *)destination;
    const unsigned char *src = (const unsigned char *)source;
    int i;

    for (i = 0; i < length; ++i)
        dst[i] = src[i];
}

int mem_eq_byte(const void *first, const void *second, int length)
{
    const unsigned char *a = (const unsigned char *)first;
    const unsigned char *b = (const unsigned char *)second;
    int i;

    for (i = 0; i < length; ++i)
        if (a[i] != b[i])
            return 0;
    return 1;
}

int abs_int32(int value)
{
    if (value >= 0)
        return value;
    if ((unsigned int)value == 0x80000000u)
        return 0x7fffffff;
    return -value;
}
