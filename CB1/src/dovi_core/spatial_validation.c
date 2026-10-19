/* Check spatial filter identifiers against mode-specific tables. */
#include "spatial_validation.h"

#define ROW_ZERO { {0, -1}, {0, -1}, {0, -1} }
#define ROW_INVALID { {-1}, {-1}, {-1} }
const int ver_spatial_filter_allowed[8][3][8] = {
    ROW_ZERO,
    { {0, 7, -1}, {0, -1}, {0, -1} },
    { {1, 7, -1}, {1, -1}, {1, -1} },
    { {1, 7, -1}, {1, -1}, {1, -1} },
    ROW_INVALID, ROW_INVALID, ROW_INVALID, ROW_INVALID,
};
const int hor_spatial_filter_allowed[8][3][8] = {
    ROW_ZERO,
    { {0, 7, -1}, {0, -1}, {0, -1} },
    ROW_INVALID, ROW_INVALID, ROW_INVALID, ROW_INVALID,
    ROW_INVALID, ROW_INVALID,
};
#undef ROW_ZERO
#undef ROW_INVALID

int validate_spatial_filter(const void *rpu_data, rpu_log_fn logger,
                               unsigned int severity)
{
    const unsigned char *header;
    unsigned char *pointer_bytes = (unsigned char *)&header;
    unsigned int index, component, vertical_mode;
    for (index = 0; index < sizeof(header); ++index)
        pointer_bytes[index] = ((const unsigned char *)rpu_data)[index];
    vertical_mode = header[0xec];
    for (component = 0; component < 3; ++component) {
        const int *allowed = ver_spatial_filter_allowed[vertical_mode][component];
        unsigned int value = header[0xed + component];
        while (*allowed != -1 && (unsigned int)*allowed != value)
            ++allowed;
        if (*allowed == -1) {
            if (logger != 0 && severity > 2u) {
                logger("Invalid vertical filter for this resampling mode\n");
                logger("Component: %d filter idc: %d mode idc: %d\n",
                       component, (unsigned int)header[0xed + component],
                       (unsigned int)header[0xec]);
            }
            return -6;
        }
        allowed = hor_spatial_filter_allowed[header[0xe8]][component];
        value = header[0xe9 + component];
        while (*allowed != -1 && (unsigned int)*allowed != value)
            ++allowed;
        if (*allowed == -1) {
            if (logger != 0 && severity > 2u) {
                logger("Invalid horizontal filter for this resampling mode\n");
                logger("Component: %d filter idc: %d mode idc: %d\n",
                       component, (unsigned int)header[0xe9 + component],
                       (unsigned int)header[0xe8]);
            }
            return -6;
        }
    }
    return 0;
}
