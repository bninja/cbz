#include <memory.h>
#include <stdlib.h>

#include "util.h"

// log levels

jobject levelno_to_levelenum(JNIEnv *env, int levelno) {
    jclass cls;
    jfieldID fid;
    char *name;
    jobject levelenum;

    cls = (*env)->FindClass(env, "com/cbz/Logger$Level");
    switch (levelno) {
        case CBZ_LOG_DBG:
            name = "DEBUG";
            break;
        case CBZ_LOG_INFO:
            name = "INFO";
            break;
        case CBZ_LOG_WARN:
            name = "WARNING";
            break;
        case CBZ_LOG_ERR:
            name = "ERROR";
            break;
        default:
            name = "INFO";
            break;
    };
    fid = (*env)->GetStaticFieldID(env, cls, name, "Lcom/cbz/Logger$Level;");
    levelenum = (*env)->GetStaticObjectField(env, cls, fid);

    return levelenum;
}

// errors

jobject errno_to_errenum(JNIEnv *env, int errno) {
    jclass cls;
    jfieldID fid;
    char *name;
    jobject errenum;

    cls = (*env)->FindClass(env, "com/cbz/Result");
    switch (errno) {
        case CBZ_OK:
            name = "OK";
            break;
        case CBZ_ERR_MEMORY:
            name = "ERR_MEMORY";
            break;
        case CBZ_ERR_UNKNOWN:
            name = "ERR_UNKNOWN";
            break;
        case CBZ_ERR_ADDRESS:
            name = "ERR_ADDRESS";
            break;
        case CBZ_ERR_CONNECT:
            name = "ERR_CONNECT";
            break;
        case CBZ_ERR_SEND:
            name = "ERR_SEND";
            break;
        case CBZ_ERR_RECV:
            name = "ERR_RECV";
            break;
        case CBZ_ERR_TIMEOUT:
            name = "ERR_TIMEOUT";
            break;
        case CBZ_ERR_SELECT:
            name = "ERR_SELECT";
            break;
        case CBZ_ERR_MAX_MSG:
            name = "ERR_MAX_MSG";
            break;
        case CBZ_ERR_SOCKET:
            name = "ERR_SOCKET";
            break;
        default:
            name = "ERR_UNKNOWN";
            break;
    };
    fid = (*env)->GetStaticFieldID(env, cls, name, "Lcom/cbz/Result;");
    errenum = (*env)->GetStaticObjectField(env, cls, fid);

    return errenum;
}

int errenum_to_errno(JNIEnv *env, jobject err_obj) {
    jclass cls;
    jmethodID mid;
    int errno;

    cls = (*env)->FindClass(env, "com/cbz/Result");
    mid = (*env)->GetMethodID(env, cls, "errno", "()I");
    errno = (*env)->CallIntMethod(env, err_obj, mid);

    return errno;
}

// context

static void *ctx_malloc(void *handle, size_t size) {
    return malloc(size);
}

static void *ctx_calloc(void *handle, size_t num, size_t size) {
    return calloc(num, size);
}

static void *ctx_realloc(void *handle, void *p, size_t size) {
    return realloc(p, size);
}

static void ctx_free(void *handle, void *p) {
    free(p);
}

static int ctx_log(void *handle, unsigned int level, char *fmt, ... ) {
    handle_t *h = (handle_t *)handle;
    jclass cls;
    jmethodID mid;
    jfieldID fid;
    va_list args;
    char *buffer = NULL;
    int len, cbz_res = CBZ_OK;
    jstring msg_obj;
    jobject level_obj, logger_obj;

    // logger

    cls = (*h->env)->FindClass(h->env, "com/cbz/Context");
    fid = (*h->env)->GetFieldID(h->env, cls, "logger", "Lcom/cbz/Logger;");
    logger_obj = (*h->env)->GetObjectField(h->env, h->ctx_obj, fid);
    if (logger_obj == NULL) {
        goto cleanup;
    }

    // level

    level_obj = levelno_to_levelenum(h->env, level);

    // message

    va_start(args, fmt);
    len = vsnprintf(NULL, 0 , fmt, args);
    va_end(args);

    buffer = malloc(len + 1);
    if (buffer == NULL) {
        cbz_res = CBZ_ERR_MEMORY;
        goto cleanup;
    }

    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    msg_obj = (*h->env)->NewStringUTF(h->env, buffer);
    if (msg_obj == NULL) {
        cbz_res = CBZ_ERR_MEMORY;
        goto cleanup;
    }

    // log

    cls = (*h->env)->FindClass(h->env, "com/cbz/Logger");
    mid = (*h->env)->GetMethodID(
                h->env,
                cls,
                "log",
                "(Lcom/cbz/Logger$Level;Ljava/lang/String;)V"
                );
    (*h->env)->CallVoidMethod(h->env, logger_obj, mid, level_obj, msg_obj);

cleanup:

    if (buffer != NULL) {
        ctx_free(handle, buffer);
        buffer = NULL;
    }

    return cbz_res;
}

void ctx_init(JNIEnv *env, handle_t *handle, cbz_ctx_t *ctx) {
    jclass cls;
    jfieldID fid;

    ctx->handle = (void *) handle;
    cls = (*env)->FindClass(env, "com/cbz/Context");
    fid = (*env)->GetFieldID(env, cls, "max_message_length", "I");
    ctx->max_msg_len = (*env)->GetIntField(env, handle->ctx_obj, fid);
    ctx->malloc = &ctx_malloc;
    ctx->calloc = &ctx_calloc;
    ctx->realloc = &ctx_realloc;
    ctx->free = &ctx_free;
    ctx->log = &ctx_log;
}
