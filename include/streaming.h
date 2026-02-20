#ifndef STREAMING_H
#define STREAMING_H

#include <stdint.h>
#include <stddef.h>

/* USB 1.1 workaround: ~12 Mbit/s. We never assume huge contiguous I/O.
 * Chunked reads/writes + double/triple buffer so we fill one while
 * the other is in use. */

#define STREAMING_CHUNK_SIZE     4096   /* 4KB per chunk */
#define STREAMING_QUEUE_DEPTH    8
#define STREAMING_BUFFER_COUNT   2     /* double buffer */

typedef enum {
    STREAM_OP_READ = 0,
    STREAM_OP_WRITE,
} stream_op_t;

typedef struct {
    stream_op_t op;
    uint32_t offset;        /* byte offset in stream */
    uint32_t length;       /* bytes to transfer */
    void *buffer;          /* must be at least STREAMING_CHUNK_SIZE */
    int done;              /* 1 when transfer complete */
    int error;             /* 0 ok, <0 error */
} stream_request_t;

typedef struct stream_pipeline stream_pipeline_t;

/* Create pipeline for path (e.g. "mass0:file.bin"). Uses chunked I/O. */
stream_pipeline_t *stream_open(const char *path, int for_write);

/* Close and flush. */
void stream_close(stream_pipeline_t *s);

/* Queue a read: offset + length, buffer. Does chunked read (4KB at a time). */
int stream_read_async(stream_pipeline_t *s, uint32_t offset, uint32_t length, void *buffer);

/* Queue a write. */
int stream_write_async(stream_pipeline_t *s, uint32_t offset, uint32_t length, const void *buffer);

/* Poll: process one chunk of pending I/O. Returns 1 if progress, 0 if idle. */
int stream_poll(stream_pipeline_t *s);

/* Block until request is done (poll in loop). */
void stream_wait(stream_pipeline_t *s, stream_request_t *req);

/* Sync read (async + wait). */
int stream_read_sync(stream_pipeline_t *s, uint32_t offset, uint32_t length, void *buffer);

/* Sync write. */
int stream_write_sync(stream_pipeline_t *s, uint32_t offset, uint32_t length, const void *buffer);

/* ----- Gameplay streaming (framebuffer -> PC) ----- */
#define STREAMING_FRAME_QUALITY_HIGH   2
#define STREAMING_FRAME_QUALITY_MED    1
#define STREAMING_FRAME_QUALITY_LOW    0
#define STREAMING_PASSTHROUGH_OFF      0
#define STREAMING_PASSTHROUGH_ON       1

int streaming_init(void);
int streaming_start(const char *client_ip);
void streaming_stop(void);
int streaming_capture_and_send(void);
void streaming_set_quality(int quality);
void streaming_set_passthrough(int on);
int streaming_active(void);

#endif /* STREAMING_H */
