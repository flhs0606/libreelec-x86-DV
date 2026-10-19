/* Decoder fields used by the state setters. */
#ifndef AM6B_RPU_SETTERS_H
#define AM6B_RPU_SETTERS_H

typedef enum {
    S_RPU_EMERGENCY = 0,
    S_RPU_ALERT = 1,
    S_RPU_CRITICAL = 2,
    S_RPU_ERROR = 3,
    S_RPU_WARNING = 4,
    S_RPU_NOTICE = 5,
    S_RPU_INFORMATIONAL = 6,
    S_RPU_DEBUG = 7,
    S_RPU_NEVERPRINT = 8,
} rpu_print_severity_t;

typedef int (*rpu_msg_log_func_t)(const char *format, ...);
typedef void (*dec_event_callback_t)(rpu_print_severity_t severity,
                                        unsigned int code, char *message);

struct rpu_setter_view {
    unsigned char before_last[0x28];
    int last; /* +0x28 */
    unsigned char before_msg_log[202140];
    rpu_msg_log_func_t msg_log; /* +0x315c8 */
    rpu_print_severity_t print_severity; /* +0x315d0 */
    unsigned char before_event_callback[260];
    dec_event_callback_t event_callback; /* +0x316d8 */
    unsigned char after_event_callback[8];
};

typedef char setters_pointer_is_eight_bytes[(sizeof(void *) == 8) ? 1 : -1];
typedef char setters_int_is_four_bytes[(sizeof(int) == 4) ? 1 : -1];
typedef char setters_severity_is_four_bytes[
    (sizeof(rpu_print_severity_t) == 4) ? 1 : -1];
typedef char setters_view_is_202472_bytes[
    (sizeof(struct rpu_setter_view) == 202472) ? 1 : -1];
#if defined(__GNUC__) || defined(__clang__)
typedef char setters_last_offset[
    (__builtin_offsetof(struct rpu_setter_view, last) == 0x28) ? 1 : -1];
typedef char setters_msg_log_offset[
    (__builtin_offsetof(struct rpu_setter_view, msg_log) == 0x315c8) ? 1 : -1];
typedef char setters_severity_offset[
    (__builtin_offsetof(struct rpu_setter_view, print_severity) == 0x315d0) ? 1 : -1];
typedef char setters_event_offset[
    (__builtin_offsetof(struct rpu_setter_view, event_callback) == 0x316d8) ? 1 : -1];
#endif

int rpu_decoder_flush(struct rpu_setter_view *rdc);
void rpu_decoder_set_msg_log(struct rpu_setter_view *rdc,
                                rpu_msg_log_func_t msg_log,
                                rpu_print_severity_t print_severity);
void register_dec_event_callback(struct rpu_setter_view *rdc,
                                    dec_event_callback_t call_back_handler);

#endif
