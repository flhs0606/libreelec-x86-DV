#ifndef DOVI_CONTROL_LUMINANCE_H
#define DOVI_CONTROL_LUMINANCE_H

/* Control-local PQ helpers. These are different from decoder-local LToPQ12.
 * Luminance uses unsigned Q18 units; PQ uses its 12-bit code.
 * PQ input clamps to 4095. L-to-PQ retains 16-bit wrapping and may
 * return 4096 at the top table endpoint; no decoder saturation is applied. */
unsigned int control_pq12_to_l(unsigned short pq);
unsigned short control_l_to_pq12(unsigned int luminance);

#endif
