#include <stdlib.h>

#include "cbz.h"

#include "com_cbz_Node.h"
#include "util.h"


JNIEXPORT void JNICALL Java_com_cbz_Node_disconnect
    (JNIEnv *env, jobject node_obj) {

    handle_t handle = { env, NULL };
    jclass cls;
    jfieldID fid;
    cbz_ctx_t ctx;
    cbz_node_t *node;

    cls = (*env)->FindClass(env, "com/cbz/Node");
    fid = (*env)->GetFieldID(env, cls, "handle", "Lcom/cbz/Context;");
    handle.ctx_obj = ((*env)->GetObjectField(env, node_obj, fid));

    ctx_init(env, &handle, &ctx);

    cls = (*env)->FindClass(env, "com/cbz/Node");
    fid = (*env)->GetFieldID(env, cls, "handle", "J");
    node = (cbz_node_t *)(*env)->GetLongField(env, node_obj, fid);

    if (node !=  NULL) {
        cbz_disconnect(&ctx, node);
        node = NULL;
        (*env)->SetLongField(env, node_obj, fid, NULL);
    }
}
