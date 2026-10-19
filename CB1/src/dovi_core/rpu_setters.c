/* Set decoder output, event and logging state. */
#include "rpu_setters.h"

int rpu_decoder_flush(struct rpu_setter_view *rdc)
{
    rdc->last = 1;
    return 0;
}

void rpu_decoder_set_msg_log(struct rpu_setter_view *rdc,
                                rpu_msg_log_func_t msg_log,
                                rpu_print_severity_t print_severity)
{
    rdc->msg_log = msg_log;
    rdc->print_severity = print_severity;
}

void register_dec_event_callback(struct rpu_setter_view *rdc,
                                    dec_event_callback_t call_back_handler)
{
    rdc->event_callback = call_back_handler;
}
