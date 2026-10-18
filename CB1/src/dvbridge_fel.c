/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Split and hardware-decode the enhancement layer; pair surfaces by exact presentation time. */
#include "dvbridge_fel.h"
#include <inttypes.h>
#include <libavcodec/bsf.h>
#include <libavutil/hwcontext.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_MAX 32
#define PACKET_BYTES_MAX (64u * 1024u * 1024u)
#define MAP_MAX (QUEUE_MAX * 2)

/* Independently coded layers can have different decode
 * order and IDR boundaries. Use FFmpeg's HEVC parser rather than assigning
 * the BL packet timestamp blindly to the encapsulated EL access unit. */
struct picture_order {
    AVCodecParserContext *parser;
    AVCodecContext *context;
    int64_t offset, maximum;
    bool seen;
};

static bool has_nal63(const uint8_t *data, int size)
{
    for (int i = 0; i < size - 4; ++i) {
        if (data[i] == 0 && data[i+1] == 0) {
            if (data[i+2] == 1 && ((data[i+3] >> 1) & 0x3f) == 63)
                return true;
            if (i < size - 5 && data[i+2] == 0 && data[i+3] == 1 && ((data[i+4] >> 1) & 0x3f) == 63)
                return true;
        }
    }
    return false;
}

static bool picture_identity(struct picture_order *order, const AVPacket *packet, int64_t *identity)
{
    if (!order || !order->parser || !order->context || !packet)
        return false;
    uint8_t *output = NULL;
    int output_size = 0;
    order->parser->output_picture_number = INT_MIN;
    int used = av_parser_parse2(order->parser, order->context, &output, &output_size,
                               packet->data, packet->size, packet->pts, packet->dts, packet->pos);
    int poc = order->parser->output_picture_number;
    if (used != packet->size || output_size <= 0 || poc == INT_MIN)
        return false;
    if (order->seen && poc == 0 && order->parser->key_frame == 1) {
        if (order->maximum == INT64_MAX)
            return false;
        order->offset = order->maximum + 1;
    }
    if (poc > 0 && order->offset > INT64_MAX - poc)
        return false;
    *identity = order->offset + poc;
    if (!order->seen || *identity > order->maximum)
        order->maximum = *identity;
    order->seen = true;
    return true;
}

struct dvbridge_fel {
    AVBSFContext *split;
    AVCodecContext *decoder;
    AVPacket *packets[QUEUE_MAX];
    AVFrame *frames[QUEUE_MAX];
    unsigned num_packets, num_frames;
    size_t packet_bytes;
    bool opened, failed, draining, sent_eof, eof;
    struct picture_order base_order, enhancement_order;
    int64_t packet_order[QUEUE_MAX];
    struct { int64_t order, pts; } times[MAP_MAX];
    unsigned num_times;
    bool synchronizing;
    int64_t anchor_pts;
    unsigned preroll_packets;
};

static enum AVPixelFormat hardware_format(AVCodecContext *ctx, const enum AVPixelFormat *formats)
{
    bool found = false;
    for (; *formats != AV_PIX_FMT_NONE; ++formats)
        found |= *formats == AV_PIX_FMT_VAAPI;
    if (!found || !ctx->hw_device_ctx)
        return AV_PIX_FMT_NONE;
    AVBufferRef *frames = NULL;
    if (avcodec_get_hw_frames_parameters(ctx, ctx->hw_device_ctx, AV_PIX_FMT_VAAPI, &frames) < 0)
        return AV_PIX_FMT_NONE;
    AVHWFramesContext *hw = (void *)frames->data;
    hw->initial_pool_size += QUEUE_MAX + 4;
    if (av_hwframe_ctx_init(frames) < 0) {
        av_buffer_unref(&frames);
        return AV_PIX_FMT_NONE;
    }
    av_buffer_unref(&ctx->hw_frames_ctx);
    ctx->hw_frames_ctx = frames;
    return AV_PIX_FMT_VAAPI;
}

static void clear_queues(struct dvbridge_fel *f)
{
    for (unsigned i = 0; i < f->num_packets; ++i)
        av_packet_free(&f->packets[i]);
    for (unsigned i = 0; i < f->num_frames; ++i)
        av_frame_free(&f->frames[i]);
    f->num_packets = f->num_frames = 0;
    f->packet_bytes = 0;
    f->failed = f->draining = f->sent_eof = f->eof = false;
    f->num_times = 0;
}

void dvbridge_fel_reset(struct dvbridge_fel *f)
{
    if (!f)
        return;
    clear_queues(f);
    f->synchronizing = false;
    f->anchor_pts = AV_NOPTS_VALUE;
    f->preroll_packets = 0;
    struct picture_order *orders[] = {&f->base_order, &f->enhancement_order};
    for (unsigned i = 0; i < 2; ++i) {
        struct picture_order *order = orders[i];
        av_parser_close(order->parser);
        order->parser = av_parser_init(AV_CODEC_ID_HEVC);
        if (order->parser)
            order->parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
        order->offset = order->maximum = 0;
        order->seen = false;
    }
    if (f->split)
        av_bsf_flush(f->split);
    if (f->opened)
        avcodec_flush_buffers(f->decoder);
}

void dvbridge_fel_destroy(struct dvbridge_fel *f)
{
    if (!f)
        return;
    clear_queues(f);
    av_parser_close(f->base_order.parser);
    av_parser_close(f->enhancement_order.parser);
    avcodec_free_context(&f->base_order.context);
    avcodec_free_context(&f->enhancement_order.context);
    avcodec_free_context(&f->decoder);
    av_bsf_free(&f->split);
    free(f);
}

struct dvbridge_fel *dvbridge_fel_create(const AVCodecParameters *p, AVRational time_base)
{
    if (!p || p->codec_id != AV_CODEC_ID_HEVC || time_base.num <= 0 || time_base.den <= 0)
        return NULL;
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!codec)
        return NULL;
    struct dvbridge_fel *f = calloc(1, sizeof(*f));
    if (!f)
        return NULL;
    f->synchronizing = false;
    f->anchor_pts = AV_NOPTS_VALUE;
    if (av_bsf_list_parse_str("dovi_split=mode=el", &f->split) < 0 ||
        avcodec_parameters_copy(f->split->par_in, p) < 0)
        goto fail;
    f->split->time_base_in = time_base;
    if (av_bsf_init(f->split) < 0)
        goto fail;
    f->decoder = avcodec_alloc_context3(codec);
    if (!f->decoder || avcodec_parameters_to_context(f->decoder, f->split->par_out) < 0)
        goto fail;
    f->decoder->pkt_timebase = time_base;
    f->decoder->get_format = hardware_format;
    f->decoder->thread_count = 1;
    struct picture_order *orders[] = {&f->base_order, &f->enhancement_order};
    const AVCodecParameters *parameters[] = {p, f->split->par_out};
    for (unsigned i = 0; i < 2; ++i) {
        orders[i]->parser = av_parser_init(AV_CODEC_ID_HEVC);
        orders[i]->context = avcodec_alloc_context3(NULL);
        if (!orders[i]->parser || !orders[i]->context ||
            avcodec_parameters_to_context(orders[i]->context, parameters[i]) < 0)
            goto fail;
        orders[i]->parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
    }
    return f;
fail:
    dvbridge_fel_destroy(f);
    return NULL;
}

static bool receive_frames(struct dvbridge_fel *f)
{
    while (f->num_frames < QUEUE_MAX) {
        AVFrame *frame = av_frame_alloc();
        if (!frame)
            return false;
        int result = avcodec_receive_frame(f->decoder, frame);
        if (result < 0) {
            av_frame_free(&frame);
            if (result == AVERROR_EOF)
                f->eof = true;
            return result == AVERROR(EAGAIN) || result == AVERROR_EOF;
        }
        if (frame->pts == AV_NOPTS_VALUE)
            frame->pts = frame->best_effort_timestamp;
        if (frame->format != AV_PIX_FMT_VAAPI || frame->pts == AV_NOPTS_VALUE) {
            av_frame_free(&frame);
            continue;
        }
        bool duplicate = false;
        for (unsigned i = 0; i < f->num_frames; ++i) {
            if (f->frames[i]->pts == frame->pts) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            av_frame_free(&frame);
            continue;
        }
        f->frames[f->num_frames++] = frame;
    }
    return true;
}

static bool pump(struct dvbridge_fel *f)
{
    if (!f->opened)
        return true;
    receive_frames(f);
    while (f->num_packets && f->num_frames < QUEUE_MAX) {
        AVPacket *packet = f->packets[0];
        if (packet->pts == AV_NOPTS_VALUE) {
            unsigned match = 0;
            while (match < f->num_times && f->times[match].order != f->packet_order[0])
                ++match;
            if (match == f->num_times) {
                if (f->num_times > 0 && f->num_packets > 4)
                    match = 0;
                else if (f->draining)
                    match = 0;
                else
                    break; /* A reference EL picture can precede its corresponding BL. */
            }
            if (match < f->num_times) {
                packet->pts = f->times[match].pts;
                --f->num_times;
                memmove(f->times + match, f->times + match + 1,
                        (f->num_times - match) * sizeof(*f->times));
            }
        }
        packet->dts = AV_NOPTS_VALUE;
        int result = avcodec_send_packet(f->decoder, packet);
        if (result == AVERROR(EAGAIN))
            break;
        f->packet_bytes -= packet->size;
        av_packet_free(&packet);
        --f->num_packets;
        memmove(f->packets, f->packets + 1, f->num_packets * sizeof(*f->packets));
        memmove(f->packet_order, f->packet_order + 1, f->num_packets * sizeof(*f->packet_order));
        receive_frames(f);
    }
    if (f->draining && !f->num_packets && !f->sent_eof && f->num_frames < QUEUE_MAX) {
        int result = avcodec_send_packet(f->decoder, NULL);
        if (result != AVERROR(EAGAIN)) {
            f->sent_eof = true;
        }
        receive_frames(f);
    }
    return true;
}

bool dvbridge_fel_device(struct dvbridge_fel *f, AVBufferRef *device)
{
    if (!f || !device || ((AVHWDeviceContext *)device->data)->type != AV_HWDEVICE_TYPE_VAAPI)
        return false;
    if (f->opened)
        return f->decoder->hw_device_ctx && f->decoder->hw_device_ctx->data == device->data;
    f->decoder->hw_device_ctx = av_buffer_ref(device);
    int open_ret = avcodec_open2(f->decoder, f->decoder->codec, NULL);
    if (!f->decoder->hw_device_ctx || open_ret < 0) {
        f->failed = true;
        return false;
    }
    f->opened = true;
    return true;
}

bool dvbridge_fel_submit(struct dvbridge_fel *f, const AVPacket *packet)
{
    if (!f || !packet || packet->size <= 0 || packet->pts == AV_NOPTS_VALUE || f->draining)
        return false;

    if (f->anchor_pts != AV_NOPTS_VALUE && packet->pts < f->anchor_pts - 90000 * 2)
    {
        /* Backward jump across seamless branching: purge old packets and reset */
        f->anchor_pts = packet->pts;
        f->preroll_packets = 0;
        for (unsigned i = 0; i < f->num_packets; ++i)
            av_packet_free(&f->packets[i]);
        f->num_packets = 0;
        f->packet_bytes = 0;
        f->num_times = 0;
    }

    int64_t base_identity;
    if (!picture_identity(&f->base_order, packet, &base_identity))
    {
        base_identity = ++f->base_order.maximum;
        f->base_order.seen = true;
        av_log(f->decoder, AV_LOG_DEBUG, "DV FEL: base picture_identity fallback to %" PRId64 "\n", base_identity);
    }

    if (f->num_times >= MAP_MAX)
    {
        memmove(&f->times[0], &f->times[1], (MAP_MAX - 1) * sizeof(f->times[0]));
        f->num_times--;
    }

    unsigned time_idx = f->num_times;
    for (unsigned i = 0; i < f->num_times; ++i)
    {
        if (f->times[i].order == base_identity)
        {
            time_idx = i;
            break;
        }
    }
    f->times[time_idx].order = base_identity;
    f->times[time_idx].pts = packet->pts;
    if (time_idx == f->num_times)
        f->num_times++;

    if (!has_nal63(packet->data, packet->size)) {
        if (f->num_packets == QUEUE_MAX || packet->size <= 0 ||
            (size_t)packet->size > PACKET_BYTES_MAX - f->packet_bytes)
            return pump(f);
        AVPacket *el_copy = av_packet_clone(packet);
        if (!el_copy)
            return false;
        f->packet_order[f->num_packets] = packet->pts;
        f->packet_bytes += el_copy->size;
        f->packets[f->num_packets++] = el_copy;
        return pump(f);
    }

    AVPacket *copy = av_packet_clone(packet);
    if (!copy)
        return false;
    int result = av_bsf_send_packet(f->split, copy);
    av_packet_free(&copy);
    if (result < 0)
        return false;

    while (true) {
        copy = av_packet_alloc();
        if (!copy)
            break;
        result = av_bsf_receive_packet(f->split, copy);
        if (result < 0) {
            av_packet_free(&copy);
            if (result == AVERROR(EAGAIN))
                return pump(f);
            return false;
        }
        if (f->num_packets == QUEUE_MAX || copy->size <= 0 ||
            (size_t)copy->size > PACKET_BYTES_MAX - f->packet_bytes) {
            av_packet_free(&copy);
            break;
        }
        int64_t el_identity;
        if (!picture_identity(&f->enhancement_order, copy, &el_identity)) {
            el_identity = ++f->enhancement_order.maximum;
            f->enhancement_order.seen = true;
            av_log(f->decoder, AV_LOG_DEBUG, "DV FEL: enhancement picture_identity fallback to %" PRId64 "\n", el_identity);
        }
        f->packet_order[f->num_packets] = el_identity;

        if (f->synchronizing) {
            const AVCodecParserContext *bp = f->base_order.parser;
            const AVCodecParserContext *ep = f->enhancement_order.parser;
            bool common_key = bp && ep && bp->key_frame == 1 && ep->key_frame == 1 &&
                              bp->output_picture_number == ep->output_picture_number;
            bool el_idr = ep && ep->key_frame == 1 && ep->output_picture_number == 0 &&
                          f->base_order.context && f->base_order.context->has_b_frames == 0;
            if (!common_key && !el_idr && (packet->flags & AV_PKT_FLAG_KEY) == 0) {
                av_packet_free(&copy);
                f->num_times = 0;
                continue;
            }
            f->enhancement_order.offset = base_identity - (ep ? ep->output_picture_number : 0);
            f->enhancement_order.maximum = base_identity;
            f->packet_order[f->num_packets] = base_identity;
            f->anchor_pts = packet->pts;
            f->synchronizing = false;
        }
        f->packet_bytes += copy->size;
        f->packets[f->num_packets++] = copy;
    }
    return pump(f);
}

bool dvbridge_fel_drain(struct dvbridge_fel *f)
{
    if (!f)
        return false;
    f->draining = true;
    return pump(f);
}

int dvbridge_fel_take(struct dvbridge_fel *f, int64_t pts, AVFrame **frame)
{
    if (!f || !frame || *frame || pts == AV_NOPTS_VALUE)
        return -1;

    pump(f);

    if (f->anchor_pts != AV_NOPTS_VALUE && pts < f->anchor_pts && f->anchor_pts - pts > 90000 * 2)
    {
        /* Backward jump across seamless branching clip: purge old segment frames */
        f->anchor_pts = pts;
        for (unsigned i = 0; i < f->num_frames; ++i)
            av_frame_free(&f->frames[i]);
        f->num_frames = 0;
    }

    for (unsigned i = 0; i < f->num_frames;) {
        AVFrame *candidate = f->frames[i];
        /* 1. Clean up stale frames that are earlier than current BL PTS */
        if (candidate->pts != AV_NOPTS_VALUE && candidate->pts + 2000 < pts) {
            av_frame_free(&f->frames[i]);
            memmove(&f->frames[i], &f->frames[i + 1],
                    (f->num_frames - i - 1) * sizeof(AVFrame *));
            --f->num_frames;
            continue;
        }
        /* 2. Candidate frame is in the future, continue checking */
        if (candidate->pts > pts + 2000) {
            ++i;
            continue;
        }
        /* 3. Matched (|candidate->pts - pts| <= 2000), extract and return 1 */
        candidate->pts = pts;
        *frame = candidate;
        memmove(&f->frames[i], &f->frames[i + 1],
                (f->num_frames - i - 1) * sizeof(AVFrame *));
        --f->num_frames;
        return 1;
    }

    return (f->eof || f->num_frames == QUEUE_MAX) ? -1 : 0;
}

bool dvbridge_fel_failed(const struct dvbridge_fel *f)
{
    return !f || f->failed;
}

bool dvbridge_fel_exhausted(const struct dvbridge_fel *f)
{
    return f && !f->failed && f->eof && !f->num_frames && !f->num_packets;
}

unsigned dvbridge_fel_num_frames(const struct dvbridge_fel *f)
{
    return f ? f->num_frames : 0;
}

int64_t dvbridge_fel_first_frame_pts(const struct dvbridge_fel *f)
{
    return (f && f->num_frames > 0 && f->frames[0]) ? f->frames[0]->pts : AV_NOPTS_VALUE;
}
