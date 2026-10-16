#ifndef DOVI_PARSER_DIAGNOSTICS_H
#define DOVI_PARSER_DIAGNOSTICS_H

/* The opaque handle must point to the parser context layout. */
int dv_md_parser_get_last_err(const void *handle);
const char *dv_md_parser_get_errstr(int error_code);
void *dv_md_parser_get_rpu_dcd_ctx(const void *handle);
const char *rpu_error_code_2_str(int error_code);

#endif
