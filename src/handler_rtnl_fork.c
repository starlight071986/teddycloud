#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "handler_rtnl_fork.h"
#include "handler_rtnl.h"
#include "fs_port.h"
#include "fs_ext.h"
#include "mutex_manager.h"
#include "server_helpers.h"
#include "settings.h"

#define RTNL_UNKNOWN_LOG_NAME "rtnl_unknown.log"
#define RTNL_UNKNOWN_LOG_MAX_SIZE (1024 * 1024) /* rotate to .1 when exceeded */
#define RTNL_UNKNOWN_PAYLOAD_MAX 512            /* bytes of payload written per line */

typedef struct
{
    uint32_t group;
    uint32_t function;
} rtnl_fork_known_t;

/* log2 group/function pairs evaluated in rtnlEvent() (handler_rtnl.c) */
static const rtnl_fork_known_t rtnl_known_log2[] = {
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_VALID_CC3200},
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_VALID_REMOVED_CC3200},
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_INVALID_CC3200},
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_INVALID_ESP32},
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_VALID_ESP32},
    {RTNL2_FUGR_TAG, RTNL2_FUNC_TAG_VALID_REMOVED_ESP32},
    {RTNL2_FUGR_AUDIO_A, RTNL2_FUNC_AUDIO_ID_CC3200},
    {RTNL2_FUGR_AUDIO_A, RTNL2_FUNC_AUDIO_ID_ESP32},
    {RTNL2_FUGR_AUDIO_B, RTNL2_FUNC_AUDIO_ID},
    {RTNL2_FUGR_AUDIO_B, RTNL2_FUNC_AUDIO_PLAY},
    {RTNL2_FUGR_TILT, RTNL2_FUNC_TILT_A_ESP32},
    {RTNL2_FUGR_TILT, RTNL2_FUNC_TILT_B_ESP32},
    {RTNL2_FUGR_VOLUME, RTNL2_FUNC_VOLUME_CHANGE_CC3200},
    {RTNL2_FUGR_VOLUME, RTNL2_FUNC_VOLUME_CHANGE_ESP32},
    {RTNL2_FUGR_FIRMWARE, RTNL2_FUNC_FIRMWARE_FULL_VERSION},
    {RTNL2_FUGR_FIRMWARE, RTNL2_FUNC_FIRMWARE_VERSION},
    {RTNL2_FUGR_FIRMWARE, RTNL2_FUNC_FIRMWARE_INFOS},
    {RTNL2_FUGR_NETWORK_HTTP, RTNL2_FUNC_NETWORK_REGION},
    {RTNL2_FUGR_NETWORK_HTTP, RTNL2_FUNC_NETWORK_DOMAIN},
    {RTNL2_FUGR_NETWORK_TCP, RTNL2_FUNC_NETWORK_DOMAIN},
};

/* log3 types evaluated in rtnlEvent() */
static const uint32_t rtnl_known_log3[] = {
    RTNL3_TYPE_EAR_BIG,
    RTNL3_TYPE_EAR_SMALL,
    RTNL3_TYPE_KNOCK_FORWARD,
    RTNL3_TYPE_KNOCK_BACKWARD,
    RTNL3_TYPE_TILT_FORWARD,
    RTNL3_TYPE_TILT_BACKWARD,
    RTNL3_TYPE_CHARGER_ON,
    RTNL3_TYPE_CHARGER_OFF,
    RTNL3_TYPE_PLAYBACK_STARTING,
    RTNL3_TYPE_PLAYBACK_STARTED,
    RTNL3_TYPE_PLAYBACK_STOPPED,
};

static bool rtnlForkIsKnownLog2(uint32_t group, uint32_t function)
{
    for (size_t i = 0; i < sizeof(rtnl_known_log2) / sizeof(rtnl_known_log2[0]); i++)
    {
        if (rtnl_known_log2[i].group == group && rtnl_known_log2[i].function == function)
        {
            return true;
        }
    }
    return false;
}

static bool rtnlForkIsKnownLog3(uint32_t type)
{
    for (size_t i = 0; i < sizeof(rtnl_known_log3) / sizeof(rtnl_known_log3[0]); i++)
    {
        if (rtnl_known_log3[i] == type)
        {
            return true;
        }
    }
    return false;
}

static char *rtnlForkLogPath(const char *suffix)
{
    return custom_asprintf("%s%c%s%s", get_settings()->internal.configdirfull, PATH_SEPARATOR, RTNL_UNKNOWN_LOG_NAME, suffix);
}

/* hex + printable ascii of a payload, truncated to RTNL_UNKNOWN_PAYLOAD_MAX bytes */
static void rtnlForkFormatBytes(const uint8_t *data, size_t len, char *hex, char *ascii)
{
    if (len > RTNL_UNKNOWN_PAYLOAD_MAX)
    {
        len = RTNL_UNKNOWN_PAYLOAD_MAX;
    }
    for (size_t i = 0; i < len; i++)
    {
        osSprintf(&hex[i * 2], "%02X", data[i]);
        ascii[i] = (data[i] >= 0x20 && data[i] < 0x7F && data[i] != '\t') ? (char)data[i] : '.';
    }
    hex[len * 2] = '\0';
    ascii[len] = '\0';
}

static void rtnlForkAppend(const char *line)
{
    char *path = rtnlForkLogPath("");
    char *pathOld = rtnlForkLogPath(".1");

    mutex_lock(MUTEX_RTNL_FILE);
    uint32_t size = 0;
    if (fsGetFileSize(path, &size) == NO_ERROR && size > RTNL_UNKNOWN_LOG_MAX_SIZE)
    {
        fsDeleteFile(pathOld);
        fsRenameFile(path, pathOld);
        size = 0;
    }
    bool isNew = !fsFileExists(path);
    FsFile *file = fsOpenFileEx(path, "ab");
    if (file)
    {
        if (isNew)
        {
            const char *header = "time\tbox_cn\tbox_name\ttype\tgroup\tfunction\tuptime\tsequence\tfield3\tlen\tpayload_hex\tpayload_ascii\tfield8\tfield9_hex\r\n";
            fsWriteFile(file, (void *)header, osStrlen(header));
        }
        fsWriteFile(file, (void *)line, osStrlen(line));
        fsCloseFile(file);
    }
    mutex_unlock(MUTEX_RTNL_FILE);

    osFreeMem(path);
    osFreeMem(pathOld);
}

void rtnlForkLogUnknown(TonieRtnlRPC *rpc, client_ctx_t *client_ctx)
{
    char time_str[64];
    time_format_current(time_str);
    const char *cn = client_ctx->settings->commonName ? client_ctx->settings->commonName : "";
    const char *name = client_ctx->settings->boxName ? client_ctx->settings->boxName : "";

    if (rpc->log2 && !rtnlForkIsKnownLog2(rpc->log2->function_group, rpc->log2->function))
    {
        TonieRtnlLog2 *log2 = rpc->log2;
        char *hex = osAllocMem(RTNL_UNKNOWN_PAYLOAD_MAX * 2 + 1);
        char *ascii = osAllocMem(RTNL_UNKNOWN_PAYLOAD_MAX + 1);
        char *hex9 = osAllocMem(RTNL_UNKNOWN_PAYLOAD_MAX * 2 + 1);
        char *ascii9 = osAllocMem(RTNL_UNKNOWN_PAYLOAD_MAX + 1);
        if (hex && ascii && hex9 && ascii9)
        {
            char field8[16] = "";
            rtnlForkFormatBytes(log2->field6.data, log2->field6.len, hex, ascii);
            rtnlForkFormatBytes(log2->has_field9 ? log2->field9.data : NULL, log2->has_field9 ? log2->field9.len : 0, hex9, ascii9);
            if (log2->has_field8)
            {
                osSprintf(field8, "%" PRIu32, log2->field8);
            }
            char *line = custom_asprintf("%s\t%s\t%s\tlog2\t%" PRIu32 "\t%" PRIu32 "\t%" PRIu64 "\t%" PRIu32 "\t%" PRIu32 "\t%" PRIuSIZE "\t%s\t%s\t%s\t%s\r\n",
                                         time_str, cn, name,
                                         log2->function_group, log2->function, log2->uptime, log2->sequence, log2->field3,
                                         log2->field6.len, hex, ascii, field8, hex9);
            rtnlForkAppend(line);
            osFreeMem(line);
        }
        osFreeMem(hex);
        osFreeMem(ascii);
        osFreeMem(hex9);
        osFreeMem(ascii9);
    }

    if (rpc->log3 && !rtnlForkIsKnownLog3(rpc->log3->field2))
    {
        char *line = custom_asprintf("%s\t%s\t%s\tlog3\t\t%" PRIu32 "\t\t\t\t\t%08" PRIX32 "\t\t\t\r\n",
                                     time_str, cn, name, rpc->log3->field2, rpc->log3->datetime);
        rtnlForkAppend(line);
        osFreeMem(line);
    }
}

static error_t rtnlForkAppendFile(const char *path, char *buf, size_t bufSize, size_t *pos)
{
    FsFile *file = fsOpenFile(path, FS_FILE_MODE_READ);
    if (!file)
    {
        return ERROR_FILE_NOT_FOUND;
    }
    size_t read = 0;
    while (*pos < bufSize && fsReadFile(file, &buf[*pos], bufSize - *pos, &read) == NO_ERROR && read > 0)
    {
        *pos += read;
    }
    fsCloseFile(file);
    return NO_ERROR;
}

/* GET /api/fork/rtnl/log - rotated file (.1) followed by the current one, as plain text */
error_t handleApiRtnlUnknownLog(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx)
{
    char *path = rtnlForkLogPath("");
    char *pathOld = rtnlForkLogPath(".1");
    size_t bufSize = 2 * RTNL_UNKNOWN_LOG_MAX_SIZE + 64 * 1024;
    char *buf = osAllocMem(bufSize + 1);
    size_t pos = 0;

    if (buf)
    {
        mutex_lock(MUTEX_RTNL_FILE);
        rtnlForkAppendFile(pathOld, buf, bufSize, &pos);
        rtnlForkAppendFile(path, buf, bufSize, &pos);
        mutex_unlock(MUTEX_RTNL_FILE);
        buf[pos] = '\0';
    }
    osFreeMem(path);
    osFreeMem(pathOld);

    if (!buf)
    {
        return ERROR_OUT_OF_MEMORY;
    }
    httpPrepareHeader(connection, "text/plain; charset=utf-8", pos);
    return httpWriteResponse(connection, buf, pos, true);
}

/* POST /api/fork/rtnl/clear */
error_t handleApiRtnlUnknownClear(HttpConnection *connection, const char_t *uri, const char_t *queryString, client_ctx_t *client_ctx)
{
    char *path = rtnlForkLogPath("");
    char *pathOld = rtnlForkLogPath(".1");

    mutex_lock(MUTEX_RTNL_FILE);
    fsDeleteFile(path);
    fsDeleteFile(pathOld);
    mutex_unlock(MUTEX_RTNL_FILE);
    osFreeMem(path);
    osFreeMem(pathOld);

    TRACE_INFO("RTNL unknown log cleared\r\n");
    char *response = "{\"success\":true}";
    httpPrepareHeader(connection, "application/json; charset=utf-8", osStrlen(response));
    return httpWriteResponseString(connection, response, false);
}
