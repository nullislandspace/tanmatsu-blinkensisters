// BlinkenSisters - Hunt for the Lost Pixels - Tanmatsu Port
//
// Tanmatsu-specific globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

// ---- Tanmatsu Build Flags ----
#define TANMATSU_BUILD 1
#define DISABLE_NETWORK 1
#define DISABLE_SOUND_STUB 0  // We have real audio via PAL
//#define DISABLE_BACKGROUND_ART 1  // Uncomment to save memory by disabling background art
#define ALLOW_BOSSKEY 1

// App paths
#define TANMATSU_APP_PATH "/sd/apps/at.cavac.blinkensisters"
#define TANMATSU_WORK_DIR "/sd/blinkensisters"

// Version
#define VERSION "0.5.4"

// Release mode
#define CAVAC_RELEASEMODE 1

// Allow cheats
#define ALLOW_CHEATCODES
#define CHEATCODECHARS "abcdefghijklmnopqrstuvwxyz0123456789 "
#define NAMECODECHARS  "abcdefghijklmnopqrstuvwxyz0123456789 "

// Display coordinates (none)
// #define DISPLAY_PLAYERCOORDS

// Bosskey - F1 returns to launcher (handled in PAL input)
#define CHECK_BOSSKEY  // no-op; handled by PAL input (F1 -> bsp_device_restart_to_launcher)

// Screen
#define SCR_WIDTH  800
#define SCR_HEIGHT 480
#define DEFSCR_WIDTH  800
#define DEFSCR_HEIGHT 480
#define TILESIZE 32

// Physics
#define PHYSICSFPS 100
#define BLINKSPEED 2
#define SPECIALTILEDELAY 50

// Player movement
#define SPRITE_SWITCHWAIT 100
#define SPRITE_MAXSPEED 5
#define SPRITE_ACCEL 0.1
#define SPRITE_JUMPSPEED -4
#define SPRITE_GRAVITY 0.09
#define SPRITE_FALLSILENTTHRESHOLD 1.0

// Scores
#define PIXEL_SCORE 20
#define EXTRAPOINTS_VALUE 150

// Max players
#define MAX_PLAYERS 1

// Misc
#define SHOW_INLAY
#define MAX_FNAME_LENGTH 500
#define MAX_STRING_LENGTH 1000
#define MAX_MONSTER_TYPES 5
#define MONSTER_DIESPEED 5
#define ELEVATOR_STICKYRANGE 7
#define CHEATTIMEOUT 10000
#define ALLOW_SMOOTHPANNING
#define SMOOTHPANNING_FACTOR 24.0
#define MAX_FGOBJECTS 1000
#define MAX_FGOBJECTGFX 2000
#define PSEUDO_FGOBJECTGFXNUM 99000000
#define MAX_FX_SAMPLES 100
#define MAX_TRIGGERS 1000

#ifndef AUDIOBUFFERSIZE
#define AUDIOBUFFERSIZE 1024
#endif

// Resource path - assets on SD card
#define RESPATH TANMATSU_APP_PATH "/"

// Addon local path
#define ADDON_LOCAL_PATH "/sd/blinkensisters/addons/"
#define ADDONHTTPTOCFILE "toc"

// Include PAL headers (replace SDL)
#include "pal/pal_types.h"
#include "pal/pal_surface.h"
#include "pal/pal_screen.h"
#include "pal/pal_time.h"
#include "pal/pal_input.h"
#include "pal/pal_font.h"
#include "shared/errorhandler.h"
#include "shared/config.h"

// Global screen surface (defined in pal_screen.cpp)
// extern BS_Surface* gScreen; -- already declared in pal_screen.h

// Last iteration's tick
extern Uint32 gLastTick;

// Fullscreen stub
#define ENABLE_FULLSCREEN
extern bool isFullscreen;

extern bool enableColor3D;
extern bool enableQuickDraw;

// Sound OK flag
extern bool soundOK;

// Cheat code
extern char cheatCode[MAX_STRING_LENGTH];
extern bool enteringCheat;

// Network stubs
#define HIGHSCOREURL ""
static char _proxyurl_stub[] = "";
#define proxyurl _proxyurl_stub

// Move types
typedef enum _MOVE_TYPE {
    MOVE_LEFT,
    MOVE_RIGHT,
    JUMP_LEFT,
    JUMP_RIGHT,
    NO_MOVE
} MOVE_TYPE;

#endif // GLOBALS_H
