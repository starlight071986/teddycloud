#ifndef _HANDLER_API_FORK_H
#define _HANDLER_API_FORK_H

#include "handler.h"

error_t handleApiBoxCmd(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx);

#endif
