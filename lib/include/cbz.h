/*
 * CozyBuzz.
 */
#ifndef _CBZ_H_INCLUDED_
#define _CBZ_H_INCLUDED_

#include <stddef.h>
#include <time.h>

#define CBZ_VERSION "0.0.0"

#define CBZ_OK                 0
#define CBZ_ERR_MEMORY        -1
#define CBZ_ERR_UNKNOWN        -2
#define CBZ_ERR_ADDRESS        -3
#define CBZ_ERR_CONNECT        -4
#define CBZ_ERR_SEND        -5
#define CBZ_ERR_RECV        -6
#define CBZ_ERR_TIMEOUT        -7
#define CBZ_ERR_SELECT        -8
#define CBZ_ERR_MAX_MSG        -9
#define CBZ_ERR_SOCKET        -10

#define CBZ_SUCCEEDED(result)    (CBZ_OK <= result)
#define CBZ_FAILED(result)         (!CBZ_SUCCEEDED(result))

#define CBZ_LOG_DBG        10
#define CBZ_LOG_INFO    20
#define CBZ_LOG_WARN    30
#define CBZ_LOG_ERR     40

#define CBZ_MAX_ADDRESS_LEN 64

typedef void *(*cbz_malloc_t)(void *handle, size_t size);
typedef void *(*cbz_calloc_t)(void *handle, size_t num, size_t size);
typedef void *(*cbz_realloc_t)(void *handle, void *p, size_t size);
typedef void (*cbz_free_t)(void *handle, void *p);
typedef int (*cbz_log_t)(void *handle, unsigned int level, char *fmt, ... );

typedef struct cbz_ctx_s {
    void *handle;
    size_t max_msg_len;
    cbz_malloc_t malloc;
    cbz_calloc_t calloc;
    cbz_realloc_t realloc;
    cbz_free_t free;
    cbz_log_t log;
} cbz_ctx_t;

typedef struct cbz_node_s cbz_node_t;

typedef struct cbz_cxn_s {
    char address[CBZ_MAX_ADDRESS_LEN + 1];
    int port;
    int result;
    cbz_node_t *node;
} cbz_cxn_t;

typedef struct cbz_ping_s {
    cbz_node_t *node;
    int result;
} cbz_ping_t;

typedef struct cbz_pong_s {
    cbz_node_t *node;
    int result;
    char *msg;
    size_t msg_len;
} cbz_pong_t;

/*
 * Connect to a collection of peers.
 */
int cbz_connect(
        cbz_ctx_t *ctx,
        cbz_cxn_t *cxns, size_t num_cxns,
        time_t timeout);

/*
 * Disconnect from a peer.
 */
void cbz_disconnect(cbz_ctx_t *ctx, cbz_node_t *node);

/*
 * Send pings to a collection of peers.
 */
int cbz_ping(
        cbz_ctx_t *ctx,
        char *msg, size_t msg_len,
        cbz_ping_t *pings, size_t num_pings,
        time_t timeout);

/*
 * Receive pongs from a collection of peers with outstanding pings.
 */
int cbz_pong(
        cbz_ctx_t *ctx,
        cbz_pong_t *pongs, size_t num_pongs,
        time_t timeout);


/*
 * Discard resources associated with a received pong.
 */
void cbz_close(cbz_ctx_t *ctx, cbz_pong_t *pong);

#endif
