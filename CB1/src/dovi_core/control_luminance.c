/* Convert between control-path Q18 luminance and 12-bit PQ codes. */
#include "control_luminance.h"
#include "control_target_tables.inc"

typedef char require_u32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char require_u64[(sizeof(unsigned long long) == 8) ? 1 : -1];
typedef char require_u16[(sizeof(unsigned short) == 2) ? 1 : -1];

unsigned int control_pq12_to_l(unsigned short pq)
{
    unsigned int value = pq, index, result;
    unsigned long long product;
    if (value > 4095u)
        value = 4095u;
    index = value >> 2;
    result = control_depq_lut[index];
    if (index != 1023u) {
        product = (unsigned long long)(control_depq_lut[index + 1u] - result) * (value & 3u);
        result += (unsigned int)(product >> 2);
    }
    return result;
}

unsigned short control_l_to_pq12(unsigned int luminance)
{
    unsigned int index = 0u, step, candidate, result;
    unsigned long long product;
    for (step = 64u; step != 0u; step >>= 1) {
        candidate = index + step;
        if (luminance >= control_l2pq_x[candidate]) {
            index = candidate;
            if (luminance == control_l2pq_x[candidate])
                break;
        }
    }
    product = (unsigned long long)(luminance - control_l2pq_x[index]) * control_l2pq_a[index];
    /* UMULL followed by ASR treats bit63 as a sign. Only low bits survive
     * the following 32-bit add and UXTH, so logical shift is equivalent. */
    result = ((control_l2pq_b[index] + (unsigned int)(product >> 25)) & 0xffffu) + 8u;
    return (unsigned short)(result >> 4);
}
