/* Convert decoder luminance into a saturated 12-bit PQ code. */
#include "luminance_pq.h"
#include "luminance_tables.inc"

typedef char require_u32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char require_u64[(sizeof(unsigned long long) == 8) ? 1 : -1];

unsigned int luminance_to_pq12(unsigned int value)
{
    unsigned int index = 0, step, candidate, result;
    unsigned long long product;
    for (step = 64; step != 0; step >>= 1) {
        candidate = index + step;
        if (value >= l2pq_lut_x[candidate]) {
            index = candidate;
            if (value == l2pq_lut_x[candidate])
                break;
        }
    }
    product = (unsigned long long)(value - l2pq_lut_x[index]) *
              l2pq_lut_a[index];
    result = ((l2pq_lut_b[index] + (unsigned int)(product >> 25)) &
              0xffffu) + 8u;
    result >>= 4;
    return result == 0x1000u ? 0xfffu : result;
}
