#ifndef _CBZJNI_UTIL_H_INCLUDED_
#define _CBZJNI_UTIL_H_INCLUDED_

#include <cbz.h>
#include <jni.h>


typedef struct handle_s {
    JNIEnv *env;
    jobject ctx_obj;
} handle_t;

void ctx_init(JNIEnv *env, handle_t *handle, cbz_ctx_t *ctx);

jobject errno_to_errenum(JNIEnv *env, int errno);

int errenum_to_errno(JNIEnv *env, jobject err_obj);

#endif
