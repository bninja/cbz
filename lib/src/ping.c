#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include "cbz.h"

#include "node.h"
#include "util.h"

static char TERMINAL[] = {'\r', '\n'};
static size_t TERMINAL_SIZE = sizeof(TERMINAL);
#define MAX_SEND 1024

#define DONE(p)		(p.curr == TERMINAL && p.curr_off == p.curr_end)
#define FAILED(p)	CBZ_FAILED(p.result)
#define PENDING(p)	(p.node != NULL && !FAILED(p) && !DONE(p))

typedef struct ping_s {
	cbz_node_t *node;
	int result;
	char *curr;
	char *curr_off;
	char *curr_end;
	size_t sent;
} ping_t;

static int _ping(cbz_ctx_t *ctx, ping_t *ping, char *msg, size_t msg_len)
{
	size_t remaining, sent, total = 0;

	if (ping->curr == NULL) {
		ping->curr = ping->curr_off = msg;
		ping->curr_end = msg + msg_len;
	}

	remaining = MIN(MAX_SEND, ping->curr_end - ping->curr_off);
	if (remaining == 0) {
		if (ping->curr_off == ping->curr_end && ping->curr == msg) {
			ping->curr = ping->curr_off = TERMINAL;
			ping->curr_end = ping->curr + sizeof(TERMINAL);
			total += _ping(ctx, ping, msg, msg_len);
		}
		return total;
	}

	sent = send(ping->node->socket, ping->curr_off, remaining, 0);
	if (sent == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG_ERR("sent() -1, errno %i", errno);
			ping->result = CBZ_ERR_SEND;
		}
		return total;
	}
	total += sent;
	ping->curr_off += sent;

	if (ping->curr_off == ping->curr_end && ping->curr == msg) {
		ping->curr = ping->curr_off = TERMINAL;
		ping->curr_end = ping->curr + sizeof(TERMINAL);
		total += _ping(ctx, ping, msg, msg_len);
	}

	return total;
}

int cbz_ping(
    cbz_ctx_t *ctx,
    char *msg, size_t msg_len,
    cbz_ping_t *pings, size_t num_pings,
    time_t timeout)
{
	time_t end, now;
	int result = CBZ_OK;
	size_t i, pending, sent;
	ping_t *ps, *p;
	fd_set write_fds;
	int max_fd, res;
	struct timeval select_timeout, *pselect_timeout = NULL;

	if (timeout != 0) {
		pselect_timeout = &select_timeout;
		time(&now);
		end = now + timeout;
	}

	ps = NEWA(num_pings, ping_t);
	if (ps == NULL)
		return CBZ_ERR_MEMORY;
	for (i = 0; i < num_pings; i += 1) {
		ps[i].node = pings[i].node;
	}

	while (true) {
		// what's left
		pending = 0;
		FD_ZERO(&write_fds);
		max_fd = 0;
		for (i = 0; i < num_pings; i += 1) {
			if (!PENDING(ps[i]))
				continue;
			pending += 1;
			FD_SET(ps[i].node->socket, &write_fds);
			max_fd = MAX(ps[i].node->socket, max_fd);
		}

		// all done?
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
		res = select(max_fd + 1, NULL, &write_fds, NULL, pselect_timeout);
		if (res == -1) {
			LOG_ERR("unable to wait, select() %i, errno %i", res, errno);
			result = CBZ_ERR_SELECT;
			break;
		}

		// write
		for (i = 0; i < num_pings; i += 1) {
			if (!PENDING(ps[i]) || !FD_ISSET(ps[i].node->socket, &write_fds))
				continue;
			_ping(ctx, &ps[i], msg, msg_len);
		}
	}

	for (i = 0; i < num_pings; i += 1) {
		pings[i].result = PENDING(ps[i]) ? CBZ_ERR_TIMEOUT : ps[i].result;
	}
	FREE(ps);

	return result;
}
