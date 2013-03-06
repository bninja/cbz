#include <stdlib.h>
#include <string.h>

#include "cbz.h"

#include "com_cbz_Context.h"
#include "util.h"

JNIEXPORT jobjectArray JNICALL Java_com_cbz_Context_connect
  (JNIEnv *env, jobject ctx_obj, jobjectArray host_objs, jint timeout) {

    handle_t handle = { env, ctx_obj };
    jclass cls;
    jfieldID address_fid, port_fid;
    jstring address_obj;
    jsize address_len;
    const char *address_utf;
    jobject host_obj;
    cbz_ctx_t ctx;
    cbz_cxn_t *cxns;
    size_t i, num_cxns;
    int cbz_res;
    jobjectArray node_objs;
    jobject node_obj, result_obj;
    jmethodID node_init_mid;

    ctx_init(env, &handle, &ctx);

    num_cxns = (*env)->GetArrayLength(env, host_objs);
    cxns = calloc(num_cxns, sizeof(cbz_cxn_t));
    if (cxns == NULL) {
        cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        (*env)->ThrowNew(env, cls, "calloc()");
        goto cleanup;
    }

    cls = (*env)->FindClass(env, "com/cbz/Host");
    address_fid = (*env)->GetFieldID(env, cls, "address", "Ljava/lang/String;");
    port_fid = (*env)->GetFieldID(env, cls, "port", "I");
    for (i = 0; i < num_cxns; i += 1) {
        host_obj = (*env)->GetObjectArrayElement(env, host_objs, i);

        // address
        address_obj = (*env)->GetObjectField(env, host_obj, address_fid);
        address_len = (*env)->GetStringUTFLength(env, address_obj);
        if (address_len > CBZ_MAX_ADDRESS_LEN) {
            cls = (*env)->FindClass(env, "java/lang/RuntimeException");
            (*env)->ThrowNew(env, cls, "Invalid address");
            goto cleanup;
        }
        address_utf = (*env)->GetStringUTFChars(env, address_obj, NULL);
        memcpy(cxns[i].address, address_utf, address_len);
        (*env)->ReleaseStringUTFChars(env, address_obj, address_utf);

        // port
        cxns[i].port = (*env)->GetIntField(env, host_obj, port_fid);
    }

    cbz_res = cbz_connect(&ctx, cxns, num_cxns, timeout);

    cls = (*env)->FindClass(env, "com/cbz/Node");
    node_objs = (*env)->NewObjectArray(env, num_cxns, cls, NULL);
    if (node_objs == NULL)
        goto cleanup;
    node_init_mid = (*env)->GetMethodID(
            env,
            cls,
            "<init>",
            "(Lcom/cbz/Host;Lcom/cbz/Result;Lcom/cbz/Context;J)V"
            );
    for (i = 0; i < num_cxns; i += 1) {
        result_obj = errno_to_errenum(env, cxns[i].result);
        node_obj = (*env)->NewObject(
                env,
                cls,
                node_init_mid,
                host_obj, result_obj, ctx_obj, cxns[i].node);
        if (node_objs == NULL)
            goto cleanup;
        (*env)->SetObjectArrayElement(env, node_objs, i, node_obj);
    }

cleanup:

    if (cxns != NULL) {
        free(cxns);
        cxns = NULL;
    }

    return node_objs;
}

JNIEXPORT jobjectArray JNICALL Java_com_cbz_Context_ping
  (JNIEnv *env, jobject ctx_obj, jobjectArray node_objs, jstring msg_obj, jint timeout) {

    handle_t handle = { env, ctx_obj };
    cbz_ctx_t ctx;
    cbz_ping_t *pings;
    size_t num_pings, i, msg_len;
    char *msg_utf;
    jclass cls;
    jfieldID handle_fid;
    jobject node_obj, ping_obj, result_obj;
    jobjectArray ping_objs;
    jmethodID ping_init_mid;
    int cbz_res;

    ctx_init(env, &handle, &ctx);

    num_pings = (*env)->GetArrayLength(env, node_objs);
    pings = calloc(num_pings, sizeof(cbz_ping_t));
    if (pings == NULL) {
        cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        (*env)->ThrowNew(env, cls, "calloc()");
        goto cleanup;
    }

    cls = (*env)->FindClass(env, "com/cbz/Node");
    handle_fid = (*env)->GetFieldID(env, cls, "handle", "J");
    for (i = 0; i < num_pings; i += 1) {
        node_obj = (*env)->GetObjectArrayElement(env, node_objs, i);
        pings[i].node = (cbz_node_t *)((*env)->GetLongField(env, node_obj, handle_fid));
    }

    msg_utf = (char *)(*env)->GetStringUTFChars(env, msg_obj, NULL);
    msg_len = (*env)->GetStringUTFLength(env, msg_obj);
    cbz_res = cbz_ping(&ctx, msg_utf, msg_len, pings, num_pings, timeout);
    (*env)->ReleaseStringUTFChars(env, msg_obj, msg_utf);

    cls = (*env)->FindClass(env, "com/cbz/Ping");
    ping_objs = (*env)->NewObjectArray(env, num_pings, cls, NULL);
    ping_init_mid = (*env)->GetMethodID(
            env,
            cls,
            "<init>",
            "(Lcom/cbz/Result;)V"
            );
    for (i = 0; i < num_pings; i += 1) {
        result_obj = errno_to_errenum(env, pings[i].result);
        ping_obj = (*env)->NewObject(env, cls, ping_init_mid, result_obj);
        if (ping_obj == NULL)
            goto cleanup;
        (*env)->SetObjectArrayElement(env, ping_objs, i, ping_obj);
    }

cleanup:

    if (pings != NULL) {
        free(pings);
        pings = NULL;
    }

    return ping_objs;
}

JNIEXPORT jobjectArray JNICALL Java_com_cbz_Context_pong
  (JNIEnv *env, jobject ctx_obj, jobjectArray node_objs, jint timeout) {

    handle_t handle = { env, ctx_obj };
    cbz_ctx_t ctx;
    cbz_pong_t *pongs;
    jclass cls, str_cls;
    jfieldID handle_fid;
    size_t num_pongs, i;
    jobject node_obj, pong_obj, result_obj, msg_obj;
    jobjectArray pong_objs;
    jmethodID pong_init_mid, str_init_mid;
    int cbz_res;
    jbyteArray buffer_obj;
    char *buffer;

    ctx_init(env, &handle, &ctx);

    num_pongs = (*env)->GetArrayLength(env, node_objs);
    pongs = calloc(num_pongs, sizeof(cbz_pong_t));
    if (pongs == NULL) {
        cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        (*env)->ThrowNew(env, cls, "calloc()");
        goto cleanup;
    }

    cls = (*env)->FindClass(env, "com/cbz/Node");
    handle_fid = (*env)->GetFieldID(env, cls, "handle", "J");
    for (i = 0; i < num_pongs; i += 1) {
        node_obj = (*env)->GetObjectArrayElement(env, node_objs, i);
        pongs[i].node = (cbz_node_t *)((*env)->GetLongField(env, node_obj, handle_fid));
    }

    cbz_res = cbz_pong(&ctx, pongs, num_pongs, timeout);

    cls = (*env)->FindClass(env, "com/cbz/Pong");
    pong_objs = (*env)->NewObjectArray(env, num_pongs, cls, NULL);
    if (pong_objs == NULL)
        goto cleanup;
    pong_init_mid = (*env)->GetMethodID(
            env,
            cls,
            "<init>",
            "(Lcom/cbz/Result;Ljava/lang/String;)V"
            );
    str_cls = (*env)->FindClass(env, "java/lang/String");
    str_init_mid = (*env)->GetMethodID(env, str_cls, "<init>", "([B)V");
    for (i = 0; i < num_pongs; i += 1) {
        // message
        buffer_obj = (*env)->NewByteArray(env, pongs[i].msg_len);
        if (buffer_obj == NULL)
            goto cleanup;
        buffer = (*env)->GetByteArrayElements(env, buffer_obj, NULL);
        memcpy(buffer, pongs[i].msg, pongs[i].msg_len);
        (*env)->ReleaseByteArrayElements(env, buffer_obj, buffer, 0);
        msg_obj = (*env)->NewObject(env, str_cls, str_init_mid, buffer_obj);
        if (msg_obj == NULL)
            goto cleanup;
        (*env)->DeleteLocalRef(env, buffer_obj);

        // result
        result_obj = errno_to_errenum(env, pongs[i].result);

        pong_obj = (*env)->NewObject(env, cls, pong_init_mid, result_obj, msg_obj);
        if (pong_obj == NULL)
        	goto cleanup;
        (*env)->SetObjectArrayElement(env, pong_objs, i, pong_obj);
    }

cleanup:

    if (pongs != NULL) {
        for (i = 0; i < num_pongs; i += 1) {
            cbz_close(&ctx, pongs + i);
        }
        free(pongs);
        pongs = NULL;
    }

    return pong_objs;
}
