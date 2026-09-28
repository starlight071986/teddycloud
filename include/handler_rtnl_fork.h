#ifndef _HANDLER_RTNL_FORK_H
#define _HANDLER_RTNL_FORK_H

#include "handler.h"
#include "proto/toniebox.pb.rtnl.pb-c.h"

/* Append RTNL messages that teddyCloud does not evaluate to config/rtnl_unknown.log */
void rtnlForkLogUnknown(TonieRtnlRPC *rpc, client_ctx_t *client_ctx);

error_t handleApiRtnlUnknownLog(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx);
error_t handleApiRtnlUnknownClear(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx);

#endif
