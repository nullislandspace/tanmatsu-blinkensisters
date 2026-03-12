# BlinkenSisters Tanmatsu Port - Progress

## Status: Step 1 COMPLETE - Clean build achieved (2026-03-11)

## Completed Work

### Infrastructure
- [x] Removed WiFi/PAX-gfx from `idf_component.yml`
- [x] Rewrote `main/CMakeLists.txt` with GLOB source collection, C++-only flags via generator expressions
- [x] Added CONFIG_FATFS_USE_FASTOPEN=y, CONFIG_FATFS_MAX_FILES_OPEN=8, CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=y to sdkconfigs/tanmatsu
- [x] Copied `fastopen.c/h` from tanmatsu-hhgg (added extern "C" guards)
- [x] Copied `minimp3.h` from tanmatsu-plugin-turret
- [x] Copied `hershey_font.h` and `hershey.h` from tanmatsu-thecube
- [x] `SDL_gfxPrimitives.h` stub - pixelRGBA, circleColor, IMG_Load

### PAL Layer (`main/pal/`)
- [x] `pal_types.h` - SDL type aliases (Uint8/16/32, Sint*, SDL_Color, SDL_Rect, SDL_MUSTLOCK stubs)
- [x] `pal_surface.h` + `pal_surface.cpp` - BS_Surface (PSRAM), blit, colorkey, fill, IMG_Load stub
- [x] `pal_screen.h` + `pal_screen.cpp` - phys_fb, BS_Flip (270° rotation), bsp_display_blit
- [x] `pal_time.h` + `pal_time.cpp` - SDL_GetTicks/SDL_Delay via esp_timer/vTaskDelay
- [x] `pal_input.h` + `pal_input.cpp` - BSP navigation events → JOYSTICK_MOVE bitmask
  - Jump: UP nav or SPACE_M/SPACE_L; Action: RETURN; Pause: F1; Turbo: F2
- [x] `pal_audio.h` + `pal_audio.cpp` - minimp3 streaming task + PCM FX cache
- [x] `pal_font.h` + `pal_font.cpp` - Hershey vector font replacing SDL_ttf

### Shared Files (`main/shared/`)
- [x] `globals.h` - TANMATSU_BUILD flags, fixed 800×480, SD paths, PAL includes
- [x] `osdef.h` / `osdef.cpp` - stripped of SDL/Win32, MKDIR + BS_strdup
- [x] `bsscreen.h` / `bsscreen.cpp` - thin shim to pal_screen
- [x] `errorhandler.h` / `errorhandler.cpp` - ESP_LOGE based
- [x] `fonthandler.h` / `fonthandler.cpp` - thin shim to pal_font
- [x] `drawprimitives.h` / `drawprimitives.cpp` - BS_Surface wrappers, IMG_Load placeholder
- [x] `config.cpp` - SD card paths (/sd/blinkensisters/, /sd/apps/at.cavac.blinkensisters/)

### Game Files (`main/game/`)
- [x] `joystick.cpp` - thin shim to PAL_GetJoystickMoves
- [x] `sound.cpp` - thin shim to PAL audio functions
- [x] `menu.cpp` - removed httpclient.h, simplified to Play/Quit
- [x] `fginlay.cpp` - stubbed (promotional overlay removed per plan)

### Lua Files (`main/lua/`)
- [x] `LuaMain/lmem.cpp` - PSRAM allocator (heap_caps_malloc MALLOC_CAP_SPIRAM)
- [x] `LuaMain/liolib.cpp` - fastopen/fastclose macros, popen stubbed
- [x] `LuaMain/loadlib.cpp` - stubbed under TANMATSU_BUILD

### Entry Point
- [x] `main.cpp` - app_main, BSP init, correct audio init sequence (from tanmatsu-tadoom), game_task (32KB stack)

## Build Status
- **Clean build as of 2026-03-11**: 0xb4eb0 bytes (740KB), 65% of partition free

## Completed Work (continued)

### Image Loading (`main/shared/drawprimitives.cpp`)
- [x] BMP decoder: 24-bit and 32-bit uncompressed, top-down and bottom-up, PSRAM pixel buffer
- [x] PNG decoder: lodepng (20260119) with custom PSRAM allocators (`lodepng_malloc/realloc/free`)
- [x] JPEG decoder: ESP32-P4 hardware JPEG engine via `esp_driver_jpeg` (lazy init, BGR888 → RGBA32)
- [x] Format detection by file extension (case-insensitive)
- [x] lodepng.h + lodepng.cpp copied to `main/pal/` as single-file library
- [x] CMakeLists.txt: added `esp_driver_jpeg` to PRIV_REQUIRES, `-DLODEPNG_NO_COMPILE_ALLOCATORS`

### Image format inventory (from original game source):
- BMP: sprites, tiles, monsters (colorkey transparency via SDL_SetColorKey)
- PNG: foreground objects, fire tiles, sign tiles (alpha transparency)
- JPEG: level backgrounds, menu/gameover/highscore screens

## Next Steps

- [ ] Step 2 (verify on HW): Flash and check display/audio basics, image loading
- [ ] Step 4: Test Lua VM executes correctly on PSRAM
- [ ] Step 5: Game loop - menu renders on screen, button navigation works
- [ ] Steps 6-10: Level loading, sprites, audio, HUD, polish

## Known Limitations (to fix for full gameplay)
1. Audio FX loading: reads MP3 from SD but no FX files preloaded at startup yet
2. First-run BMF extraction: untested on hardware (extractmetabmf.cpp + config.cpp)
