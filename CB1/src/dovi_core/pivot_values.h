#ifndef AM6B_PIVOT_VALUES_H
#define AM6B_PIVOT_VALUES_H

struct pivot_rpu_root {
    unsigned char *header;
};

/* Header points to a writable little-endian RPU header. */
int assign_pivot_values(struct pivot_rpu_root *root);
int assign_nlq_pivot_values(struct pivot_rpu_root *root);

#endif
