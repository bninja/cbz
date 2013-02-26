from libc.string cimport strncpy, memset 

cdef extern from "stdarg.h":

    ctypedef struct va_list:
        pass
    ctypedef struct fake_type:
        pass
    void va_start(va_list, void* arg)
    void* va_arg(va_list, fake_type)
    void va_end(va_list)
    fake_type int_type "int"

cdef extern from "Python.h":

    void* PyMem_Malloc(size_t n)
    void* PyMem_Realloc(void *p, size_t n)
    void PyMem_Free(void *p)
    object PyString_FromFormatV(const char *format, va_list vargs)


cdef void* PyMem_Calloc(size_t num, size_t size):
    cdef void *p
    cdef size_t total = num * size
     
    p = PyMem_Malloc(total)
    if (p != NULL):
        memset(p, 0, total)
    return p
    

cdef extern from "cbz.h":

    cdef char* CBZ_VERSION
    
    cdef int CBZ_OK
    cdef int CBZ_ERR_MEMORY
    cdef int CBZ_ERR_UNKNOWN
    cdef int CBZ_ERR_ADDRESS
    cdef int CBZ_ERR_CONNECT
    cdef int CBZ_ERR_SEND
    cdef int CBZ_ERR_RECV
    cdef int CBZ_ERR_TIMEOUT
    cdef int CBZ_ERR_SELECT
    cdef int CBZ_ERR_MAX_MSG
    cdef int CBZ_ERR_SOCKET
    
    int CBZ_SUCCEEDED(int)
    int CBZ_FAILED(int)
    
    cdef int CBZ_LOG_DBG
    cdef int CBZ_LOG_INFO
    cdef int CBZ_LOG_WARN
    cdef int CBZ_LOG_ERR
    
    cdef int CBZ_MAX_ADDRESS_LEN
    
    ctypedef void *(*cbz_malloc_t)(void *handle,  size_t size)
    ctypedef void *(*cbz_calloc_t)(void *handle,  size_t num, size_t size)
    ctypedef void *(*cbz_realloc_t)(void *handle,  void *p, size_t size)
    ctypedef void (*cbz_free_t)(void *handle,  void *p)
    ctypedef int (*cbz_log_t)(void *handle,  unsigned int level, char *fmt, ... )
    
    ctypedef struct cbz_ctx_t:
        void *handle
        size_t max_msg_len
        cbz_malloc_t malloc
        cbz_calloc_t calloc
        cbz_realloc_t realloc
        cbz_free_t free
        cbz_log_t log
        
    ctypedef struct cbz_node_t:
        pass
    
    ctypedef struct cbz_cxn_t:
        char *address
        int port
        int result
        cbz_node_t *node
        
    ctypedef struct cbz_ping_t:
        cbz_node_t *node
        int result
    
    ctypedef struct cbz_pong_t:
        cbz_node_t *node
        int result
        char *msg
        size_t msg_len
        
    int cbz_connect(
            cbz_ctx_t *ctx,
            cbz_cxn_t *cxns, size_t num_cxns,
            int timeout)
    
    void cbz_disconnect(cbz_ctx_t *ctx, cbz_node_t *node)

    int cbz_ping(
            cbz_ctx_t *ctx,
            char *msg, size_t msg_len,
            cbz_ping_t *pings, size_t num_pings,
            int timeout)

    int cbz_pong(
            cbz_ctx_t *ctx,
            cbz_pong_t *pongs, size_t num_pongs,
            int timeout)

cdef void* ctx_malloc(void *handle, size_t size):
    return PyMem_Malloc(size);

cdef void* ctx_calloc(void *handle, size_t num, size_t size):
    return PyMem_Calloc(num, size);

cdef void* ctx_realloc(void *handle, void *p, size_t size):
    return PyMem_Realloc(p, size);

cdef void ctx_free(void *handle, void *p):
    PyMem_Free(p);

cdef int ctx_log(void *handle, unsigned int level, char *fmt, ... ):
    cdef va_list args
    va_start(args, fmt);
    s = PyString_FromFormatV(fmt, args)
    c = (<Context>handle)
    if c.log != None:
        c.log(level, s)
    return OK

VERSION = CBZ_VERSION

OK = CBZ_OK
ERR_MEMORY = CBZ_ERR_MEMORY
ERR_UNKNOWN = CBZ_ERR_UNKNOWN
ERR_ADDRESS = CBZ_ERR_ADDRESS
ERR_CONNECT = CBZ_ERR_CONNECT
ERR_SEND = CBZ_ERR_SEND
ERR_RECV = CBZ_ERR_RECV
ERR_TIMEOUT = CBZ_ERR_TIMEOUT
ERR_SELECT = CBZ_ERR_SELECT
ERR_MAX_MSG = CBZ_ERR_MAX_MSG
ERR_SOCKET = CBZ_ERR_SOCKET

LOG_DBG = CBZ_LOG_DBG
LOG_INFO = CBZ_LOG_INFO
LOG_WARN = CBZ_LOG_WARN
LOG_ERR = CBZ_LOG_ERR

class Error(Exception):
    
    DESCRIPTIONS = {
        ERR_MEMORY: ('MEMORY', 'Memory allocation failure'),
        ERR_UNKNOWN: ('UNKNOWN', 'Unknown'),
        ERR_ADDRESS: ('ADDRESS', 'Invalid address'),
        ERR_CONNECT: ('CONNECT', 'Connection failed'),
        ERR_SEND: ('SEND', 'Send message failed'),
        ERR_RECV: ('RECV', 'Receive message failed'),
        ERR_TIMEOUT: ('TIMEOUT', 'Timeout expired'),
        ERR_SELECT: ('SELECT', 'Select poll failed'),
        ERR_MAX_MSG: ('MAX_MSG', 'Exceeded maximum message length'),
        ERR_SOCKET: ('SOCKET', 'Socket creation failed'),
        }
    
    def __init__(self, errno, results):
        name, detail = self.DESCRIPTIONS.get(errno, self.DESCRIPTIONS[ERR_UNKNOWN]) 
        super(Error, self).__init__(
            '{0} ({1}) - {2}'.format(name, errno, detail))
        self.errno = errno
        self.results = results

cdef class Context:

    cdef cbz_ctx_t ctx
    cdef object log
    
    def __init__(self, max_msg_len=0, log=None):
        self.ctx.handle = <void *>self
        self.ctx.max_msg_len = max_msg_len
        self.ctx.malloc = ctx_malloc
        self.ctx.calloc = ctx_calloc
        self.ctx.realloc = ctx_realloc
        self.ctx.free = ctx_free 
        self.ctx.log = ctx_log
        self.log = log
        
    def connect(self, peers, timeout=0):
        if len(peers) == 0:
            return []

        cdef int result = CBZ_OK
        cdef size_t num_cxns = len(peers)
        cdef cbz_cxn_t *cxns = <cbz_cxn_t *>PyMem_Calloc(num_cxns, sizeof(cbz_cxn_t))
        cdef Node node
        
        if cxns == NULL:
            raise MemoryError()

        try:
            for i, (address, port) in enumerate(peers):
                strncpy(cxns[i].address, address, len(address))
                cxns[i].address[len(address)] = '\0'
                cxns[i].port = <int>port
    
            result = cbz_connect(&(self.ctx), cxns, num_cxns, timeout)

            results = []
            for i, (address, port) in enumerate(peers):
                if CBZ_SUCCEEDED(cxns[i].result):
                    node = Node(self, address, port)
                    node.node = cxns[i].node
                else:
                    if cxns[i].node != NULL:
                        cbz_disconnect(&(self.ctx), cxns[i].node)
                        cxns[i].node = NULL
                    node = None
                results.append((cxns[i].result, node))
        finally:
            PyMem_Free(cxns)

        if CBZ_FAILED(result):
            raise Error(result, results)
        return results

    def ping(self, nodes, msg, timeout=0):
        if len(nodes) == 0:
            return []
        
        cdef int result = CBZ_OK
        cdef size_t num_pings = len(nodes)
        cdef cbz_ping_t *pings = <cbz_ping_t *>PyMem_Calloc(num_pings, sizeof(cbz_ping_t))
        
        if pings == NULL:
            raise MemoryError()
    
        try:
            for i in range(num_pings):
                pings[i].node = (<Node?>nodes[i]).node
    
            result = cbz_ping(
                        &(self.ctx),
                        <bytes?>msg, len(msg),
                        pings, num_pings,
                        timeout);

            results = [(pings[i].result, nodes[i]) for i in range(num_pings)]
        finally:
            PyMem_Free(pings)
            
        if CBZ_FAILED(result):
            Error(result, results)
        return results
    
    def pong(self, nodes, timeout=0):
        if len(nodes) == 0:
            return []

        cdef int result = CBZ_OK
        cdef size_t num_pongs = len(nodes)
        cdef cbz_pong_t *pongs = <cbz_pong_t *>PyMem_Calloc(num_pongs, sizeof(cbz_pong_t))

        if pongs == NULL:
            raise MemoryError()
        
        try:
            for i in range(num_pongs):
                pongs[i].node = (<Node?>nodes[i]).node

            result = cbz_pong(
                        &(self.ctx),
                        <cbz_pong_t *>pongs, num_pongs,
                        timeout);

            results = [
                (pongs[i].result, pongs[i].msg[:pongs[i].msg_len], nodes[i])
                for i in range(num_pongs)
                ]
        finally:
            PyMem_Free(pongs)

        if CBZ_FAILED(result):
            Error(result, results)
        return results

cdef class Node:
    
    cdef readonly bytes address
    cdef readonly int port
    cdef Context ctx
    cdef cbz_node_t *node
    
    def __init__(self, ctx, address, port):
        self.ctx = ctx
        self.address = <bytes>address
        self.port = port
        self.node = NULL
    
    def __dealloc__(self):
        self.disconnect()
        
    def ping(self, msg, timeout=0):
        if not self.connected:
            raise Exception("Disconnected")
        results = self.ctx.ping([self], msg, timeout)
        result, _ = results[0]
        if CBZ_FAILED(result):
            raise Error(result, results)
        return result
    
    def pong(self, timeout=0):
        if not self.connected:
            raise Exception("Disconnected")
        results = self.ctx.pong([self], timeout)
        result, msg, _ = results[0]
        if CBZ_FAILED(result):
            raise Error(result, results)
        return (result, msg)
        
    property connected:

        def __get__(self):
            return self.node != NULL
        
            
    def disconnect(self):
        if self.connected:
            cbz_disconnect(&(self.ctx.ctx), self.node)
            self.node = NULL
            self.ctx = None
