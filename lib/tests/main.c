#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cbz.h"

void* ctx_malloc(void *handle, size_t size)
{
    return malloc(size);
}

void* ctx_calloc(void *handle, size_t num, size_t size)
{
    return calloc(num, size);
}

void* ctx_realloc(void *handle, void *p, size_t size)
{
	return realloc(p, size);
}

void ctx_free(void *handle, void *p)
{
	free(p);
}

int ctx_log(void *handle, unsigned int level, char *fmt, ...)
{
	va_list args;

	switch(level) {
		case CBZ_LOG_INFO:
			fprintf(stderr, "[info] ");
			break;
		case CBZ_LOG_WARN:
			fprintf(stderr, "[warning] ");
			break;
		case CBZ_LOG_ERR:
			fprintf(stderr, "[error] ");
			break;
		case CBZ_LOG_DBG:
		default:
			fprintf(stderr, "[debug] ");
			break;
	}
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (strlen(fmt) == 0 || fmt[strlen(fmt) - 1] != '\n')
		fprintf(stderr, "\n");
	return CBZ_OK;
}

cbz_ctx_t ctx = {
	NULL,
	0,
	&ctx_malloc,
	&ctx_calloc,
	&ctx_realloc,
	&ctx_free,
	&ctx_log,
};

typedef void (*test_t)(cbz_cxn_t *, size_t);

void test_cxn_fail(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i;
	int res;

	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);

	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_ERR_CONNECT);
		assert(cxns[i].node != NULL);
		cbz_disconnect(&ctx, cxns[i].node);
	}
}

void test_pong_timeout(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i;
	int res;
	cbz_ping_t *pings;
	cbz_pong_t *pongs;

	// connect
	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_OK);
		assert(cxns[i].node != NULL);
	}

	// ping
	pings = calloc(num_cxns, sizeof(cbz_ping_t));
	for (i = 0; i < num_cxns; i += 1) {
		pings[i].node = cxns[i].node;
	}
	res = cbz_ping(&ctx, "ping", sizeof("ping") - 1, pings, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pings[i].result == CBZ_OK);
	}

	// pong
	pongs = calloc(num_cxns, sizeof(cbz_pong_t));
	for (i = 0; i < num_cxns; i += 1) {
		pongs[i].node = cxns[i].node;
	}
	res = cbz_pong(&ctx, pongs, num_cxns, 1);
	assert(res == CBZ_ERR_TIMEOUT);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pongs[i].result == CBZ_ERR_TIMEOUT);
		printf("pong %s:%i\n", cxns[i].address, cxns[i].port);
		printf("%.*s", (int)pongs[i].msg_len, pongs[i].msg);
		printf("\n");
		cbz_close(&ctx, &pongs[i]);
	}
	assert(res == CBZ_OK);

	// disconnect
	for (i = 0; i < num_cxns; i += 1) {
		cbz_disconnect(&ctx, cxns[i].node);
	}
}

void test_pong_max_msg(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i;
	int res;
	cbz_ping_t *pings;
	cbz_pong_t *pongs;

	// connect
	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_OK);
		assert(cxns[i].node != NULL);
	}

	// ping
	pings = calloc(num_cxns, sizeof(cbz_ping_t));
	for (i = 0; i < num_cxns; i += 1) {
		pings[i].node = cxns[i].node;
	}
	res = cbz_ping(&ctx, "ping", sizeof("ping") - 1, pings, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pings[i].result == CBZ_OK);
	}

	// pong
	pongs = calloc(num_cxns, sizeof(cbz_pong_t));
	for (i = 0; i < num_cxns; i += 1) {
		pongs[i].node = cxns[i].node;
	}
	ctx.max_msg_len = 10;
	res = cbz_pong(&ctx, pongs, num_cxns, 1);
	ctx.max_msg_len = 0;
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pongs[i].result == CBZ_ERR_MAX_MSG);
		printf("pong %s:%i\n", cxns[i].address, cxns[i].port);
		printf("%.*s", (int)pongs[i].msg_len, pongs[i].msg);
		printf("\n");
		cbz_close(&ctx, &pongs[i]);
	}
	assert(res == CBZ_OK);

	// disconnect
	for (i = 0; i < num_cxns; i += 1) {
		cbz_disconnect(&ctx, cxns[i].node);
	}
}

void test_ping_empty(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i;
	int res;
	cbz_ping_t *pings;
	cbz_pong_t *pongs;

	// connect
	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_OK);
		assert(cxns[i].node != NULL);
	}

	// ping
	pings = calloc(num_cxns, sizeof(cbz_ping_t));
	for (i = 0; i < num_cxns; i += 1) {
		pings[i].node = cxns[i].node;
	}
	res = cbz_ping(&ctx, "", sizeof("") - 1, pings, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pings[i].result == CBZ_OK);
	}

	// pong
	pongs = calloc(num_cxns, sizeof(cbz_pong_t));
	for (i = 0; i < num_cxns; i += 1) {
		pongs[i].node = cxns[i].node;
	}
	res = cbz_pong(&ctx, pongs, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pongs[i].result == CBZ_OK);
		printf("pong %s:%i\n", cxns[i].address, cxns[i].port);
		printf("%.*s", (int)pongs[i].msg_len, pongs[i].msg);
		printf("\n");
		cbz_close(&ctx, &pongs[i]);
	}
	assert(res == CBZ_OK);

	// disconnect
	for (i = 0; i < num_cxns; i += 1) {
		cbz_disconnect(&ctx, cxns[i].node);
	}
}

void test_ping_pong(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i;
	int res;
	cbz_ping_t *pings;
	cbz_pong_t *pongs;

	// connect
	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_OK);
		assert(cxns[i].node != NULL);
	}

	// ping
	pings = calloc(num_cxns, sizeof(cbz_ping_t));
	for (i = 0; i < num_cxns; i += 1) {
		pings[i].node = cxns[i].node;
	}
	res = cbz_ping(&ctx, "ping", sizeof("ping") - 1, pings, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pings[i].result == CBZ_OK);
	}

	// pong
	pongs = calloc(num_cxns, sizeof(cbz_pong_t));
	for (i = 0; i < num_cxns; i += 1) {
		pongs[i].node = cxns[i].node;
	}
	res = cbz_pong(&ctx, pongs, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(pongs[i].result == CBZ_OK);
		printf("pong %s:%i\n", cxns[i].address, cxns[i].port);
		printf("%.*s", (int)pongs[i].msg_len, pongs[i].msg);
		printf("\n");
		cbz_close(&ctx, &pongs[i]);
	}
	assert(res == CBZ_OK);

	// disconnect
	for (i = 0; i < num_cxns; i += 1) {
		cbz_disconnect(&ctx, cxns[i].node);
	}
}

void test_champion(cbz_cxn_t *cxns, size_t num_cxns)
{
	size_t i, active;
	int res;
	cbz_ping_t *pings;
	cbz_pong_t *pongs;

	// connect
	res = cbz_connect(&ctx, cxns, num_cxns, 0);
	assert(res == CBZ_OK);
	for (i = 0; i < num_cxns; i += 1) {
		assert(cxns[i].result == CBZ_OK);
		assert(cxns[i].node != NULL);
	}

	pings = calloc(num_cxns, sizeof(cbz_ping_t));
	pongs = calloc(num_cxns, sizeof(cbz_pong_t));
	active = num_cxns;

	while (active != 0) {
		// ping
		for (i = 0; i < num_cxns; i += 1) {
			pings[i].node = cxns[i].node;
		}
		res = cbz_ping(&ctx, "ping", sizeof("ping") - 1, pings, num_cxns, 0);
		assert(res == CBZ_OK);
		for (i = 0; i < num_cxns; i += 1) {
			if (pings[i].node != NULL && CBZ_FAILED(pings[i].result)) {
				cbz_disconnect(&ctx, cxns[i].node);
				cxns[i].node = NULL;
				active -= 1;
			}
		}

		// pong
		for (i = 0; i < num_cxns; i += 1) {
			pongs[i].node = cxns[i].node;
		}
		res = cbz_pong(&ctx, pongs, num_cxns, 0);
		assert(res == CBZ_OK);
		for (i = 0; i < num_cxns; i += 1) {
			printf("pong %s:%i\n", cxns[i].address, cxns[i].port);
			printf("%.*s", (int)pongs[i].msg_len, pongs[i].msg);
			printf("\n");
			cbz_close(&ctx, &pongs[i]);
		}
	}

	free(pings);
	free(pongs);

	// disconnect
	for (i = 0; i < num_cxns; i += 1) {
		if (cxns[i].node != NULL)
			cbz_disconnect(&ctx, cxns[i].node);
	}
}

const char* USAGE = "{command} {[address]:port} ... {[address]:port}";

#define ERR_CMD		-1
#define ERR_PEER	-2

int main(int argc, char** argv)
{
	test_t test = NULL;
	int i, res = 0;
	char *arg, *cp;
	cbz_cxn_t *cxns = NULL, *cxn;
	size_t num_cxns;

	// cmd
	if (argc == 1) {
		fprintf(stderr, "missing command, usage - %s\n", USAGE);
		res = ERR_CMD;
		goto cleanup;
	}
	arg = argv[1];
	if (strcmp(arg, "cxn-fail") == 0)
		test = &test_cxn_fail;
	else if (strcmp(arg, "pong-timeout") == 0)
		test = &test_pong_timeout;
	else if (strcmp(arg, "pong-max-msg") == 0)
		test = &test_pong_max_msg;
	else if (strcmp(arg, "ping-empty") == 0)
		test = &test_ping_empty;
	else if (strcmp(arg, "ping-pong") == 0)
		test = &test_ping_pong;
	else if (strcmp(arg, "champion") == 0)
		test = &test_champion;
	else {
		fprintf(stderr, "invalid command '%s', usage - %s\n", arg, USAGE);
		res = ERR_CMD;
		goto cleanup;
	}

	// peers
	num_cxns = argc - 2;
	cxns = calloc(num_cxns, sizeof(cbz_cxn_t));
	cxn = cxns;
	for (i = 2; i < argc; i++) {
		arg = argv[i];
		cp = strchr(arg, ':');
		if (cp == NULL) {
			fprintf(stderr, "invalid peer '%s', usage - %s\n", arg, USAGE);
			res = ERR_PEER;
			goto cleanup;
		}
		if (cp - arg == 0) {
			strcpy(cxn->address, "127.0.0.1");
		}
		else {
			memcpy(cxn->address, arg, cp - arg);
			cxn->address[cp - arg] = '\0';
		}
		cxn->port = atoi(cp + 1);
		if (cxn->port == 0) {
			fprintf(stderr, "invalid peer port '%s', usage - %s\n", arg, USAGE);
			res = ERR_PEER;
			goto cleanup;
		}
		cxn += 1;
	}

	test(cxns, num_cxns);

cleanup:
	free(cxns);

	return res;
}
