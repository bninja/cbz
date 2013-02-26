#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include "cbz.h"

#include "node.h"
#include "util.h"

#define BUF_BASE_SIZE 32
#define BUF_INC_SIZE 1024
#define MAX_RECV_SIZE 1024

static char TERMINAL[] = {'\r', '\n'};
static size_t TERMINAL_SIZE = sizeof(TERMINAL);

#define DONE(p)		(p.buf_end - p.buf_off >= sizeof(TERMINAL) && memcmp(p.buf_off, TERMINAL, sizeof(TERMINAL)) == 0)
#define FAILED(p)	CBZ_FAILED(p.result)
#define PENDING(p)	(p.node != NULL && !FAILED(p) && !DONE(p))

typedef struct pong_s {
	cbz_node_t *node;
	int result;
	char *buf;
	char *buf_off;
	char *buf_end;
} pong_t;

static int _pong(cbz_ctx_t *ctx, pong_t *pong)
{
	size_t total, remaining, size;
	ssize_t recvd;
	void *buf;

	// grow buf
	remaining = pong->buf_end - pong->buf_off;
	if (remaining == 0) {
		size = (pong->buf_off - pong->buf);
		if (ctx->max_msg_len != 0 && size >= ctx->max_msg_len) {
			pong->result = CBZ_ERR_MAX_MSG;
			return 0;
		}
		total = size;
		if (ctx->max_msg_len != 0)
			total += MIN(BUF_INC_SIZE, ctx->max_msg_len - total);
		else
			total += BUF_INC_SIZE;
		buf = REALLOC(pong->buf, total);
		if (buf == NULL) {
			pong->result = CBZ_ERR_MEMORY;
			return 0;
		}
		pong->buf = buf;
		pong->buf_off = pong->buf + size;
		pong->buf_end = pong->buf + total;
	}

	// recv to buf
	size = MIN(MAX_RECV_SIZE, pong->buf_end - pong->buf_off);
	recvd = recv(pong->node->socket, pong->buf_off, size, 0);
	if (recvd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		pong->result = CBZ_ERR_RECV;
		return 0;
	}
	if (recvd == 0) {
		pong->result = CBZ_ERR_RECV;
		return 0;
	}

	// adjust buf
	pong->buf_off += recvd;
	if (pong->buf - pong->buf_off >= sizeof(TERMINAL) &&
		memcmp(pong->buf_off - TERMINAL_SIZE, TERMINAL, TERMINAL_SIZE) == 0) {
		pong->buf_off -= TERMINAL_SIZE;
	}

	return recvd;
}

int cbz_pong(
		cbz_ctx_t *ctx,
		cbz_pong_t *pongs, size_t num_pongs,
		time_t timeout)
{
	int result;
	time_t now, end;
	size_t i, pending, recvd;
	pong_t *ps, *p;
	fd_set read_fds;
	int max_fd, res;
	struct timeval select_timeout, *pselect_timeout = NULL;

	if (timeout != 0) {
		pselect_timeout = &select_timeout;
		time(&now);
		end = now + timeout;
	}

	// state
	ps = NEWA(num_pongs, pong_t);
	if (ps == NULL)
		return CBZ_ERR_MEMORY;
	for (i = 0; i < num_pongs; i += 1) {
		ps[i].node = pongs[i].node;
		ps[i].buf = ps[i].buf_off = MALLOC(BUF_BASE_SIZE);
		if (ps[i].buf == NULL) {
			ps[i].result = CBZ_ERR_MEMORY;
			continue;
		}
		ps[i].buf_end = ps[i].buf + BUF_BASE_SIZE;
	}

	// read
	FD_ZERO(&read_fds);
	do {
		pending = 0;
		for (i = 0; i < num_pongs; i += 1) {
			if (!PENDING(ps[i]) || FD_ISSET(ps[i].node->socket, &read_fds))
				continue;
			recvd = _pong(ctx, &ps[i]);
			if (recvd == 0)
				FD_SET(ps[i].node->socket, &read_fds);
			else
				pending += 1;
		}
	} while (pending != 0);

	while (true) {
		// what's left
		pending = 0;
		FD_ZERO(&read_fds);
		max_fd = 0;
		for (i = 0; i < num_pongs; i += 1) {
			if (!PENDING(ps[i]))
				continue;
			pending += 1;
			FD_SET(ps[i].node->socket, &read_fds);
			max_fd = MAX(ps[i].node->socket, max_fd);
		}
		if (pending == 0) {
			result = CBZ_OK;
			break;
		}

		// wait
		if (pselect_timeout != NULL) {
			time(&now);
			if (now >= end) {
				result = CBZ_ERR_TIMEOUT;
				break;
			}
			pselect_timeout->tv_sec = (end - now);
			pselect_timeout->tv_usec = 0;
		}
		res = select(max_fd + 1, &read_fds, NULL, NULL, pselect_timeout);
		if (res  == -1) {
			LOG_ERR("unable to wait, select() %i, errno %i", res, errno);
			result = CBZ_ERR_SELECT;
			break;
		}

		// read
		for (i = 0; i < num_pongs; i += 1) {
			if (!PENDING(ps[i]) || !FD_ISSET(ps[i].node->socket, &read_fds))
				continue;
			_pong(ctx, &ps[i]);
		}
	}

	for (i = 0; i < num_pongs; i += 1) {
		pongs[i].result = PENDING(ps[i]) ? CBZ_ERR_TIMEOUT : ps[i].result;
		pongs[i].msg = ps[i].buf;
		pongs[i].msg_len = ps[i].buf_off - ps[i].buf;
	}
	FREE(ps);

	return result;
}


void cbz_close(cbz_ctx_t *ctx, cbz_pong_t *pong)
{
	if (pong->msg == NULL) {
		FREE(pong->msg);
		pong->msg_len = 0;
	}
}
