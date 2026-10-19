#ifndef COMPOSER_REGISTERS_H
#define COMPOSER_REGISTERS_H
#include "composer_registers_abi.h"

/* The packer takes configuration, width, height and output. Width
 * occupies the low bits; height is shifted by 13, without range masking.
 * Composer/register buffers must cover their ABI sizes and must not overlap.
 * DM registers, reserved words and unused composer fields remain untouched. */
unsigned short comp_cfg_2_comp_reg(const void *composer, void *registers,
                                 int width, int height);
#endif
