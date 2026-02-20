/* USB 1.1 workaround: chunked I/O + double buffer.
 * Plus gameplay streaming: framebuffer capture -> transport. */

#include "streaming.h"
#include "kernel.h"
#include "memory_manager.h"
#include "transport.h"
#include "video.h"
#include <stddef.h>

#define FRAME_CHUNK_HEADER 8
#define FRAME_CHUNK_PAYLOAD (TRANSPORT_MAX_PAYLOAD - FRAME_CHUNK_HEADER)

struct stream_pipeline {
    char path[128];
    int for_write;
    stream_request_t queue[STREAMING_QUEUE_DEPTH];
    int queue_head;
    int queue_tail;
    unsigned int active_buf;  /* double buffer index */
};

stream_pipeline_t *stream_open(const char *path, int for_write) {
    if (!path) return NULL;
    stream_pipeline_t *s = (stream_pipeline_t *)malloc(sizeof(stream_pipeline_t));
    if (!s) return NULL;
    unsigned int i = 0;
    while (path[i] && i < sizeof(s->path) - 1) { s->path[i] = path[i]; i++; }
    s->path[i] = '\0';
    s->for_write = for_write;
    s->queue_head = 0;
    s->queue_tail = 0;
    s->active_buf = 0;
    for (i = 0; i < STREAMING_QUEUE_DEPTH; i++) {
        s->queue[i].done = 1;
        s->queue[i].error = 0;
    }
    return s;
}

void stream_close(stream_pipeline_t *s) {
    if (s) free(s);
}

static int advance_chunk(stream_pipeline_t *s, stream_request_t *req) {
    /* Stub: no real I/O. Real impl would read/write STREAMING_CHUNK_SIZE from s->path at req->offset. */
    if (req->length <= STREAMING_CHUNK_SIZE) {
        req->done = 1;
        req->error = 0;
        return 0;
    }
    req->offset += STREAMING_CHUNK_SIZE;
    req->length -= STREAMING_CHUNK_SIZE;
    if (req->buffer) req->buffer = (char *)req->buffer + STREAMING_CHUNK_SIZE;
    return 1;
}

int stream_read_async(stream_pipeline_t *s, uint32_t offset, uint32_t length, void *buffer) {
    if (!s || !buffer) return -1;
    if (s->queue_tail >= STREAMING_QUEUE_DEPTH) return -2;
    stream_request_t *r = &s->queue[s->queue_tail];
    r->op = STREAM_OP_READ;
    r->offset = offset;
    r->length = length;
    r->buffer = buffer;
    r->done = 0;
    r->error = 0;
    s->queue_tail++;
    return 0;
}

int stream_write_async(stream_pipeline_t *s, uint32_t offset, uint32_t length, const void *buffer) {
    if (!s || !buffer) return -1;
    if (s->queue_tail >= STREAMING_QUEUE_DEPTH) return -2;
    stream_request_t *r = &s->queue[s->queue_tail];
    r->op = STREAM_OP_WRITE;
    r->offset = offset;
    r->length = length;
    r->buffer = (void *)buffer;
    r->done = 0;
    r->error = 0;
    s->queue_tail++;
    return 0;
}

int stream_poll(stream_pipeline_t *s) {
    if (!s) return 0;
    int progress = 0;
    for (int i = 0; i < s->queue_tail; i++) {
        stream_request_t *r = &s->queue[i];
        if (r->done) continue;
        progress |= advance_chunk(s, r);
    }
    return progress;
}

void stream_wait(stream_pipeline_t *s, stream_request_t *req) {
    if (!s || !req) return;
    while (!req->done) stream_poll(s);
}

int stream_read_sync(stream_pipeline_t *s, uint32_t offset, uint32_t length, void *buffer) {
    if (!s || !buffer) return -1;
    stream_request_t *r = &s->queue[0];
    r->op = STREAM_OP_READ;
    r->offset = offset;
    r->length = length;
    r->buffer = buffer;
    r->done = 0;
    r->error = 0;
    while (r->length > 0 && !r->error) {
        advance_chunk(s, r);
    }
    return r->error;
}

int stream_write_sync(stream_pipeline_t *s, uint32_t offset, uint32_t length, const void *buffer) {
    if (!s || !buffer) return -1;
    stream_request_t *r = &s->queue[0];
    r->op = STREAM_OP_WRITE;
    r->offset = offset;
    r->length = length;
    r->buffer = (void *)buffer;
    r->done = 0;
    r->error = 0;
    while (r->length > 0 && !r->error) {
        advance_chunk(s, r);
    }
    return r->error;
}

/* ----- Gameplay streaming ----- */
static int streaming_initialized;
static int streaming_running;
static int stream_quality;
static int stream_passthrough;
static uint32_t frame_id;
static uint8_t frame_chunk_buf[TRANSPORT_MAX_PAYLOAD];

int streaming_init(void) {
    if (streaming_initialized)
        return 0;
    transport_init();
    streaming_running = 0;
    stream_quality = STREAMING_FRAME_QUALITY_MED;
    stream_passthrough = STREAMING_PASSTHROUGH_OFF;
    frame_id = 0;
    streaming_initialized = 1;
    return 0;
}

int streaming_start(const char *client_ip) {
    if (!streaming_initialized)
        return -1;
    (void)client_ip;
    streaming_running = 1;
    frame_id = 0;
    return 0;
}

void streaming_stop(void) {
    streaming_running = 0;
}

static unsigned int effective_width(void) {
    int q = stream_quality;
    if (q == STREAMING_FRAME_QUALITY_LOW)
        return (unsigned int)VIDEO_WIDTH / 2;
    return (unsigned int)VIDEO_WIDTH;
}

static unsigned int effective_height(void) {
    int q = stream_quality;
    if (q == STREAMING_FRAME_QUALITY_LOW)
        return (unsigned int)VIDEO_HEIGHT / 2;
    return (unsigned int)VIDEO_HEIGHT;
}

int streaming_capture_and_send(void) {
    if (!streaming_initialized || !streaming_running)
        return -1;
    const unsigned int w = effective_width();
    const unsigned int h = effective_height();
    const unsigned int bpp = (unsigned int)VIDEO_BPP;
    const unsigned int row_bytes = w * bpp;
    const unsigned int total_bytes = row_bytes * h;
    const unsigned int n_chunks = (total_bytes + FRAME_CHUNK_PAYLOAD - 1) / FRAME_CHUNK_PAYLOAD;

    const volatile uint8_t *fb = (const volatile uint8_t *)FRAMEBUFFER_ADDR;
    unsigned int step = (stream_quality == STREAMING_FRAME_QUALITY_LOW) ? 2 : 1;
    unsigned int src_offset = 0;
    unsigned int chunk_idx = 0;
    const unsigned int full_w = (unsigned int)VIDEO_WIDTH;

    while (chunk_idx < n_chunks) {
        uint8_t *hdr = frame_chunk_buf;
        hdr[0] = (uint8_t)(frame_id & 0xFF);
        hdr[1] = (uint8_t)((frame_id >> 8) & 0xFF);
        hdr[2] = (uint8_t)((frame_id >> 16) & 0xFF);
        hdr[3] = (uint8_t)((frame_id >> 24) & 0xFF);
        hdr[4] = (uint8_t)(chunk_idx & 0xFF);
        hdr[5] = (uint8_t)((chunk_idx >> 8) & 0xFF);
        hdr[6] = (uint8_t)(n_chunks & 0xFF);
        hdr[7] = (uint8_t)((n_chunks >> 8) & 0xFF);

        unsigned int to_copy = FRAME_CHUNK_PAYLOAD;
        if (src_offset + to_copy > total_bytes)
            to_copy = total_bytes - src_offset;

        uint8_t *dst = frame_chunk_buf + FRAME_CHUNK_HEADER;
        if (step == 1) {
            for (unsigned int i = 0; i < to_copy; i++)
                dst[i] = fb[src_offset + i];
        } else {
            for (unsigned int i = 0; i < to_copy; i++) {
                unsigned int pix = src_offset + i;
                unsigned int row = (pix / bpp) / w;
                unsigned int col = (pix / bpp) % w;
                unsigned int phys = (row * 2 * full_w + col * 2) * bpp + (pix % bpp);
                dst[i] = fb[phys];
            }
        }
        src_offset += to_copy;

        if (transport_send(TRANSPORT_TYPE_DATA, frame_chunk_buf, FRAME_CHUNK_HEADER + (uint16_t)to_copy) < 0)
            return -1;
        chunk_idx++;
    }

    frame_id++;
    if (stream_passthrough == STREAMING_PASSTHROUGH_ON) {
        uint8_t type;
        uint8_t passthrough_buf[64];
        if (transport_receive(passthrough_buf, sizeof(passthrough_buf), &type) > 0 && type == TRANSPORT_TYPE_DATA)
            ;
    }
    return 0;
}

void streaming_set_quality(int quality) {
    if (quality >= STREAMING_FRAME_QUALITY_LOW && quality <= STREAMING_FRAME_QUALITY_HIGH)
        stream_quality = quality;
}

void streaming_set_passthrough(int on) {
    stream_passthrough = on ? STREAMING_PASSTHROUGH_ON : STREAMING_PASSTHROUGH_OFF;
}

int streaming_active(void) {
    return streaming_running ? 1 : 0;
}
