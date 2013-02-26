#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "cbz.h"

#include "node.h"
#include "util.h"


static int _connect(cbz_ctx_t *ctx, cbz_cxn_t *cxn)
{
	int res, flags;
	struct sockaddr_in socket_addr;

	cxn->node = NEW(cbz_cxn_t);
	if (cxn->node == NULL) {
		return CBZ_ERR_MEMORY;
	}
	cxn->node->socket = -1;

	// address
	memset(&socket_addr, 0, sizeof(socket_addr));
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(cxn->port);
	res = inet_pton(AF_INET, cxn->address, (struct sockaddr *)&socket_addr.sin_addr);
	if (res < 0) {
		LOG_DBG("invalid address %s:%i, inet_pton() %i, errno %i",
			cxn->address, cxn->port, res, errno);
		return CBZ_ERR_ADDRESS;
	}
	if (res == 0) {
		LOG_DBG("invalid address %s:%i, inet_pton() %i",
			cxn->address, cxn->port, res);
		return CBZ_ERR_ADDRESS;
	}
	strncpy(cxn->node->address, cxn->address, strlen(cxn->address));
	cxn->node->port = cxn->port;

	// socket
	cxn->node->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (cxn->node->socket == -1) {
		LOG_ERR("socket creation failed, socket() %i, errno %i",
			res, errno);
		return CBZ_ERR_SOCKET;
	}
	flags = fcntl(cxn->node->socket, F_GETFL, 0);
	res = fcntl(cxn->node->socket, F_SETFL, flags | O_NONBLOCK);
	if (res == -1) {
		LOG_ERR("unable to set non-blocking, fcntl() %i, errno %i",
			res, errno);
		return CBZ_ERR_SOCKET;
	}

	// connect ...
	res = connect(cxn->node->socket, (struct sockaddr *)&socket_addr,
			sizeof(struct sockaddr));
	if (res == -1) {
		if (errno != EINPROGRESS) {
			LOG_ERR("unable to connect to %s:%i, connect() %i, errno %i",
				cxn->address, cxn->port, res, errno);
			return CBZ_ERR_CONNECT;
		}
		cxn->node->pending = true;
	}
	else {
		LOG_INFO("connected to %s:%i", cxn->address, cxn->port);
		cxn->node->pending = false;
	}

	return CBZ_OK;
}

int cbz_connect(
		cbz_ctx_t *ctx,
		cbz_cxn_t *cxns, size_t num_cxns,
		time_t timeout)
{
	int result = CBZ_OK, flags, max_fd, res, opt, opt_len;
	size_t i, pending;
	time_t now, end;
	struct sockaddr_in socket_addr;
	fd_set write_fds;
	struct timeval select_timeout, *pselect_timeout = NULL;

	if (timeout != 0) {
		pselect_timeout = &select_timeout;
		time(&now);
		end = now + timeout;
	}

	// initiate connections
	for (i = 0; i < num_cxns; i += 1) {
		cxns[i].result = _connect(ctx, &cxns[i]);
	}

	// wait for connections to complete
	while (true) {
		// what's left
		pending = 0;
		max_fd = -1;
		FD_ZERO(&write_fds);
		for (i = 0; i < num_cxns; i += 1) {
			if (cxns[i].node->pending) {
				LOG_DBG("waiting to connect to %s:%i ...", cxns[i].address,
					cxns[i].port);
				pending += 1;
				max_fd = MAX(cxns[i].node->socket, max_fd);
				FD_SET(cxns[i].node->socket, &write_fds);
			}
		}
		if (pending == 0) {
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
		if (res  == -1) {
			LOG_ERR("unable to wait, select() %i, errno %i", res, errno);
			for (i = 0; i < num_cxns; i += 1) {
				if (cxns[i].node->pending) {
					cxns[i].result = CBZ_ERR_SELECT;
					cxns[i].node->pending = false;
				}
			}
			break;
		}
		if (res == 0) {
			result = CBZ_ERR_TIMEOUT;
			break;
		}

		// write
		for (i = 0; i < num_cxns; i += 1) {
			if (!cxns[i].node->pending ||
				!FD_ISSET(cxns[i].node->socket, &write_fds))
				continue;
			opt_len = sizeof(opt);
			if (getsockopt(cxns[i].node->socket, SOL_SOCKET, SO_ERROR, &opt,
					&opt_len) < 0) {
				LOG_ERR("unable to connect to %s:%i, getsockopt() < 0, errno %i",
					cxns[i].address, cxns[i].port, errno);
				cxns[i].result = CBZ_ERR_CONNECT;
			}
			else if (opt != 0) {
				LOG_ERR("unable to connect to %s:%i, getsockopt() %i",
					cxns[i].address, cxns[i].port, opt);
				cxns[i].result = CBZ_ERR_CONNECT;
			}
			else {
				LOG_INFO("connected to %s:%i", cxns[i].address, cxns[i].port);
			}
			cxns[i].node->pending = false;
		}
	}

	// timeouts
	for (i = 0; i < num_cxns; i += 1) {
		if (cxns[i].node->pending) {
			LOG_INFO("connection to %s:%i timed-out after %s sec(s)",
				cxns[i].address, cxns[i].port, timeout);
			cxns[i].result = CBZ_ERR_TIMEOUT;
			cxns[i].node->pending = false;
		}
	}

	return result;
}

void cbz_disconnect(cbz_ctx_t *ctx, cbz_node_t *node)
{
	if (node->socket != -1) {
		close(node->socket);
		node->socket = -1;
		LOG_INFO("disconnected from %s:%i", node->address, node->port);
	}
	FREE(node);
}
