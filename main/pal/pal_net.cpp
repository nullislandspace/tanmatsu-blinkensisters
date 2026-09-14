// BlinkenSisters - Tanmatsu port
// Network access for the addon downloader. See pal_net.h.

#include "pal_net.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

#include "fastopen.h"
#include "shared/globals.h"

extern "C" {
#include "bsp/power.h"
#include "wifi_connection.h"
#include "wifi_remote.h"
#include "wifi_settings.h"
}

static const char* TAG = "pal_net";

#define NET_CHUNK      (32 * 1024)
#define NET_TIMEOUT_MS 20000
#define NET_USER_AGENT "BlinkenSisters-Tanmatsu/" VERSION

static volatile PAL_NetState s_state = PAL_NET_UNAVAILABLE;

// ---- WiFi -------------------------------------------------------------------

void PAL_NetBootInit(void) {
    // Power-cycle the radio first, in case the previous app left it busy;
    // the launcher and tanmatsu-discord do the same.
    bsp_power_set_radio_state(BSP_POWER_RADIO_STATE_OFF);
    vTaskDelay(pdMS_TO_TICKS(200));

    if (wifi_remote_initialize() != ESP_OK) {
        bsp_power_set_radio_state(BSP_POWER_RADIO_STATE_OFF);
        ESP_LOGE(TAG, "radio not responding; no WiFi this session");
        return;
    }
    esp_err_t res = wifi_connection_init_stack();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "WiFi stack init failed: %s", esp_err_to_name(res));
        return;
    }
    s_state = PAL_NET_IDLE;
    ESP_LOGI(TAG, "WiFi stack up, free internal %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

// Runs on its own task: it reads the saved networks from NVS, which is flash,
// and flash access is not allowed from a task whose stack is in PSRAM -- as
// the game task's is. wifi_connect_try_all() also blocks for a long time, and
// its own timeout does not fire (wifi-manager 0.3.0), so the UI waits on the
// state with a deadline of its own instead.
static void connect_task(void* arg) {
    (void)arg;
    wifi_settings_t settings;
    if (wifi_settings_get(0, &settings) != ESP_OK) {
        // The launcher packs saved networks from slot 0, so an empty slot 0
        // means there are none.
        ESP_LOGW(TAG, "no WiFi networks saved in the launcher");
        s_state = PAL_NET_NO_NETWORKS;
    } else if (wifi_connect_try_all() == ESP_OK && wifi_connection_is_connected()) {
        ESP_LOGI(TAG, "WiFi connected");
        s_state = PAL_NET_CONNECTED;
    } else {
        ESP_LOGW(TAG, "could not join any saved WiFi network");
        s_state = PAL_NET_FAILED;
    }
    vTaskDelete(NULL);
}

void PAL_NetStartConnect(void) {
    if (s_state == PAL_NET_UNAVAILABLE || s_state == PAL_NET_CONNECTING) return;
    if (s_state == PAL_NET_CONNECTED && wifi_connection_is_connected()) return;
    s_state = PAL_NET_CONNECTING;
    if (xTaskCreatePinnedToCore(connect_task, "netconnect", 8192, NULL, 4, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "cannot start the connect task");
        s_state = PAL_NET_FAILED;
    }
}

PAL_NetState PAL_NetGetState(void) {
    if (s_state == PAL_NET_CONNECTED && !wifi_connection_is_connected()) {
        s_state = PAL_NET_FAILED;   // dropped since
    }
    return s_state;
}

// ---- HTTP -------------------------------------------------------------------

static void set_err(char* err, size_t errlen, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    ESP_LOGW(TAG, "%s", err);
}

static esp_http_client_handle_t make_client(const char* url) {
    esp_http_client_config_t cfg = {};
    cfg.url               = url;
    cfg.timeout_ms        = NET_TIMEOUT_MS;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.buffer_size       = 4096;
    // GitHub answers a release download with a redirect to a long signed URL;
    // the request line for it does not fit the default 512-byte TX buffer.
    cfg.buffer_size_tx    = 4096;
    cfg.keep_alive_enable = true;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client) esp_http_client_set_header(client, "User-Agent", NET_USER_AGENT);
    return client;
}

// Send the request and read the response headers, following redirects.
// Returns the final status code, or -1 with `err` set.
static int open_request(esp_http_client_handle_t client, uint64_t range_from,
                        int64_t* content_length, char* err, size_t errlen) {
    for (int redirects = 0; redirects <= 5; redirects++) {
        if (range_from) {
            char range[48];
            snprintf(range, sizeof(range), "bytes=%llu-", (unsigned long long)range_from);
            esp_http_client_set_header(client, "Range", range);
        } else {
            esp_http_client_delete_header(client, "Range");
        }
        esp_err_t res = esp_http_client_open(client, 0);
        if (res != ESP_OK) {
            set_err(err, errlen, "Cannot connect (%s)", esp_err_to_name(res));
            return -1;
        }
        *content_length = esp_http_client_fetch_headers(client);
        int status = esp_http_client_get_status_code(client);
        if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
            if (esp_http_client_set_redirection(client) != ESP_OK) {
                set_err(err, errlen, "Bad redirect (HTTP %d)", status);
                return -1;
            }
            // Drain the redirect's body before the next request on this client.
            char drain[256];
            while (esp_http_client_read(client, drain, sizeof(drain)) > 0) {}
            continue;
        }
        return status;
    }
    set_err(err, errlen, "Too many redirects");
    return -1;
}

bool PAL_NetFetch(const char* url, char** body, size_t* len, size_t maxlen,
                  char* err, size_t errlen) {
    *body = NULL;
    *len = 0;
    esp_http_client_handle_t client = make_client(url);
    if (!client) {
        set_err(err, errlen, "Out of memory");
        return false;
    }
    int64_t clen = 0;
    int status = open_request(client, 0, &clen, err, errlen);
    bool ok = false;
    char* buf = NULL;
    size_t have = 0;
    if (status == 200) {
        buf = (char*)heap_caps_malloc(maxlen + 1, MALLOC_CAP_SPIRAM);
        if (!buf) {
            set_err(err, errlen, "Out of memory");
        } else {
            ok = true;
            for (;;) {
                int n = esp_http_client_read(client, buf + have, (int)(maxlen - have));
                if (n < 0) {
                    set_err(err, errlen, "Connection lost");
                    ok = false;
                    break;
                }
                if (n == 0) break;
                have += (size_t)n;
                if (have == maxlen) {
                    set_err(err, errlen, "Response too large");
                    ok = false;
                    break;
                }
            }
            if (ok && clen > 0 && have != (size_t)clen) {
                set_err(err, errlen, "Connection lost");
                ok = false;
            }
        }
    } else if (status > 0) {
        set_err(err, errlen, "Server said HTTP %d", status);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (!ok) {
        free(buf);
        return false;
    }
    buf[have] = 0;
    *body = buf;
    *len = have;
    return true;
}

static void to_hex(const unsigned char* in, size_t n, char* out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 15];
    }
    out[2 * n] = 0;
}

bool PAL_NetDownload(const char* url, const char* dest, uint64_t size, const char* sha256hex,
                     PAL_NetProgress progress, void* ctx, char* err, size_t errlen) {
    char part[512];
    snprintf(part, sizeof(part), "%s.part", dest);

    uint8_t* buf = (uint8_t*)heap_caps_malloc(NET_CHUNK, MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
    if (!buf) {
        set_err(err, errlen, "Out of memory");
        return false;
    }
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);

    // Resume: hash what an earlier attempt already saved.
    uint64_t have = 0;
    struct stat st;
    if (stat(part, &st) == 0) {
        if (st.st_size > 0 && (uint64_t)st.st_size < size) {
            FILE* in = fastopen(part, "rb");
            size_t n;
            while (in && (n = fread(buf, 1, NET_CHUNK, in)) > 0) {
                mbedtls_sha256_update(&sha, buf, n);
                have += n;
            }
            bool readerr = !in || ferror(in);
            if (in) fastclose(in);
            if (readerr || have != (uint64_t)st.st_size) {
                have = 0;
                mbedtls_sha256_starts(&sha, 0);
            }
        }
        if (have == 0) unlink(part);
    }

    bool ok = false;
    FILE* out = NULL;
    esp_http_client_handle_t client = make_client(url);
    if (!client) {
        set_err(err, errlen, "Out of memory");
        goto done;
    }

    {
        int64_t clen = 0;
        int status = open_request(client, have, &clen, err, errlen);
        if (status < 0) goto done;
        if (have && status == 200) {
            ESP_LOGI(TAG, "server ignored the range; starting %s over", dest);
            have = 0;
            mbedtls_sha256_starts(&sha, 0);
        } else if (have && status == 416) {
            // Nothing left to send, which our size says cannot be right.
            unlink(part);
            set_err(err, errlen, "Download went wrong, please try again");
            goto done;
        } else if (status != (have ? 206 : 200)) {
            set_err(err, errlen, "Server said HTTP %d", status);
            goto done;
        }
        if (clen >= 0 && have + (uint64_t)clen != size) {
            set_err(err, errlen, "Unexpected size (%llu instead of %llu bytes); the addon list may be out of date",
                    (unsigned long long)(have + (uint64_t)clen), (unsigned long long)size);
            goto done;
        }
        if (have) ESP_LOGI(TAG, "resuming %s at %llu bytes", dest, (unsigned long long)have);

        out = fastopen(part, have ? "ab" : "wb");
        if (!out) {
            set_err(err, errlen, "Cannot write %s (errno %d)", part, errno);
            goto done;
        }
        if (progress && !progress(ctx, have, size)) {
            set_err(err, errlen, "Stopped");
            goto done;
        }
        while (have < size) {
            int n = esp_http_client_read(client, (char*)buf, NET_CHUNK);
            if (n < 0) {
                set_err(err, errlen, "Connection lost");
                goto done;
            }
            if (n == 0) {
                if (esp_http_client_is_complete_data_received(client)) break;
                set_err(err, errlen, "Connection lost");
                goto done;
            }
            if (have + (uint64_t)n > size) {
                set_err(err, errlen, "Server sent more than expected");
                goto done;
            }
            if (fwrite(buf, 1, (size_t)n, out) != (size_t)n) {
                set_err(err, errlen, "Write error on the SD card (errno %d)", errno);
                goto done;
            }
            mbedtls_sha256_update(&sha, buf, (size_t)n);
            have += (uint64_t)n;
            if (progress && !progress(ctx, have, size)) {
                set_err(err, errlen, "Stopped");
                goto done;
            }
        }
        if (fflush(out) != 0 || ferror(out)) {
            set_err(err, errlen, "Write error on the SD card (errno %d)", errno);
            goto done;
        }
        fastclose(out);
        out = NULL;
        if (have != size) {
            set_err(err, errlen, "Connection lost");
            goto done;
        }

        unsigned char digest[32];
        char hex[65];
        mbedtls_sha256_finish(&sha, digest);
        to_hex(digest, sizeof(digest), hex);
        if (strcasecmp(hex, sha256hex) != 0) {
            // Complete but wrong: resuming it could never help.
            unlink(part);
            set_err(err, errlen, "The download is corrupt (checksum mismatch)");
            goto done;
        }
        unlink(dest);
        if (rename(part, dest) != 0) {
            set_err(err, errlen, "Cannot rename %s (errno %d)", part, errno);
            goto done;
        }
        ok = true;
    }

done:
    if (out) fastclose(out);
    if (client) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }
    mbedtls_sha256_free(&sha);
    heap_caps_free(buf);
    return ok;
}
