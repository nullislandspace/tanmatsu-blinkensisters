#include "errorhandler.h"
#include "pal/pal_font.h"
#include "pal/pal_screen.h"
#include "pal/pal_time.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char* ETAG = "bserror";

#ifndef __OGGPLAYER
#include "lua/LuaMain/lua.h"
#include "lua/LuaBindings/bl_lua.h"
#endif

bool displayGraphicalErrors = false;

#define PARSE_ERRORCODES
char* getErrorText(Uint32 errorcode) {
    static char intErrString[255];
    intErrString[0] = 0;
#define XX(errenum, errcode, errtxt) \
    if (errorcode == errcode) { sprintf(intErrString, "%s", errtxt); }
#include "errorcodes.h"
    return intErrString;
}
#undef PARSE_ERRORCODES

void dieWithError(Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename) {
    ESP_LOGE(ETAG, "FATAL ERROR %d: %s | %s | %s:%d",
             errorcode, getErrorText(errorcode), extrainfo, filename, linenum);
    // Show on screen if possible
    if (gScreen) {
        SDL_FillRect(gScreen, NULL, 0xff000000); // black bg
        SDL_Color red = {0xff, 0, 0, 0};
        SDL_Color white = {0xff, 0xff, 0xff, 0};
        char buf[256];
        snprintf(buf, sizeof(buf), "FATAL ERROR %d", errorcode);
        renderFontHandlerText(0, 50, buf, red, true, false, FONT_menufont_30);
        renderFontHandlerText(0, 100, getErrorText(errorcode), white, true, false, FONT_textfont_20);
        renderFontHandlerText(0, 140, extrainfo, white, true, false, FONT_textfont_20);
        BS_Flip(gScreen);
    }
    // Halt
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}

void displayErrorOnScreen(Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename) {
    ESP_LOGE(ETAG, "Error %d: %s | %s | %s:%d",
             errorcode, getErrorText(errorcode), extrainfo, filename, linenum);
    if (gScreen) {
        SDL_Color red = {0xff, 0, 0, 0};
        SDL_Color white = {0xff, 0xff, 0xff, 0};
        char buf[256];
        snprintf(buf, sizeof(buf), "Error %d: %s", errorcode, getErrorText(errorcode));
        renderFontHandlerText(0, 200, buf, red, true, false, FONT_textfont_20);
        renderFontHandlerText(0, 230, extrainfo, white, true, false, FONT_textfont_12);
        BS_Flip(gScreen);
        SDL_Delay(2000);
    }
}

void displaymessage(int messagetype, const char* message, Uint32 delay_ms) {
    const char* prefix = messagetype == displaymessage_ERROR ? "ERROR" :
                         messagetype == displaymessage_WARNING ? "WARN" : "INFO";
    ESP_LOGI(ETAG, "[%s] %s", prefix, message);
    if (gScreen && displayGraphicalErrors) {
        SDL_Color white = {0xff, 0xff, 0xff, 0};
        SDL_Color fg = (messagetype == displaymessage_ERROR) ?
                       (SDL_Color){0xff, 0, 0, 0} :
                       (messagetype == displaymessage_WARNING) ?
                       (SDL_Color){0xff, 0x7f, 0, 0} : white;
        renderFontHandlerText(0, 200, message, fg, true, false, FONT_textfont_20);
        BS_Flip(gScreen);
    }
    if (delay_ms > 0) SDL_Delay(delay_ms);
}
