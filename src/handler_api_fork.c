#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handler_api_fork.h"
#include "server_helpers.h"
#include "settings.h"
#include "toniebox_state.h"

/* GET /api/box/cmd?boxId={commonName}&cmd={command}&value={value}
 * Same commands as the MQTT command entities (see MQTT_CONTROL.md). */
error_t handleApiBoxCmd(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx)
{
    char boxId[64] = "";
    char cmd[32] = "";
    char value[16] = "";
    char response[128];
    bool success = false;

    queryGet(queryString, "boxId", boxId, sizeof(boxId));
    queryGet(queryString, "cmd", cmd, sizeof(cmd));
    queryGet(queryString, "value", value, sizeof(value));

    uint8_t overlay_id = get_overlay_id(boxId);
    if (overlay_id == 0)
    {
        TRACE_WARNING("Box cmd: unknown boxId '%s'\r\n", boxId);
        osSnprintf(response, sizeof(response), "{\"success\":false,\"error\":\"unknown boxId\"}");
        httpPrepareHeader(connection, "application/json; charset=utf-8", osStrlen(response));
        connection->response.statusCode = 404;
        return httpWriteResponseString(connection, response, false);
    }

    uint32_t num = (uint32_t)strtoul(value, NULL, 10);
    bool flag = !strcasecmp(value, "true") || !strcmp(value, "1") || !strcasecmp(value, "on");

    if (!strcmp(cmd, "stop"))
    {
        success = tbs_cmd_stop(overlay_id);
    }
    else if (!strcmp(cmd, "volLimitSpk"))
    {
        success = tbs_cmd_set_vol_limit_spk(overlay_id, num);
    }
    else if (!strcmp(cmd, "volLimitHdp"))
    {
        success = tbs_cmd_set_vol_limit_hdp(overlay_id, num);
    }
    else if (!strcmp(cmd, "led"))
    {
        success = tbs_cmd_set_led(overlay_id, num);
    }
    else if (!strcmp(cmd, "slapEnabled"))
    {
        success = tbs_cmd_set_slap_enabled(overlay_id, flag);
    }
    else if (!strcmp(cmd, "slapDir"))
    {
        success = tbs_cmd_set_slap_dir(overlay_id, flag);
    }
    else
    {
        TRACE_WARNING("Box cmd: unknown cmd '%s'\r\n", cmd);
    }

    TRACE_INFO("Box cmd '%s' value '%s' on '%s': %s\r\n", cmd, value, boxId, success ? "OK" : "FAILED");
    osSnprintf(response, sizeof(response), "{\"success\":%s}", success ? "true" : "false");
    httpPrepareHeader(connection, "application/json; charset=utf-8", osStrlen(response));
    if (!success)
    {
        connection->response.statusCode = 400;
    }
    return httpWriteResponseString(connection, response, false);
}
