#ifndef _CBZ_NODE_H_INCLUDED_
#define _CBZ_NODE_H_INCLUDED_

#include <stdbool.h>

#include "cbz.h"

struct cbz_node_s {
    char address[CBZ_MAX_ADDRESS_LEN];
    int port;
    int socket;
    bool pending;
};

#endif
