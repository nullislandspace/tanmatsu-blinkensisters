# BlinkenSisters Tanmatsu Port - Progress

## Status: Step 7 IN PROGRESS - Gameplay testing (2026-03-12)

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
  - [x] `SDL_DisplayFormat` returns `BS_DupSurface(s)` (copy, not same pointer) — fixes dangling pointer on Quit
  - [x] `BS_BlitSurface` skips alpha=0 pixels in non-colorkey path — fixes sprite transparency
- [x] `pal_screen.h` + `pal_screen.cpp` - phys_fb, BS_Flip (270° rotation), bsp_display_blit
  - [x] Backlight enabled via `bsp_display_set_backlight_brightness(100)` in BS_InitScreen
- [x] `pal_time.h` + `pal_time.cpp` - SDL_GetTicks/SDL_Delay via esp_timer/vTaskDelay
- [x] `pal_input.h` + `pal_input.cpp` - BSP navigation events → JOYSTICK_MOVE bitmask
  - [x] Full keyboard/scancode support via BSP input events
  - [x] SDL_Event ring buffer (32 events) with PAL_PollEvent
  - [x] Scancode mapping: ESC→SDLK_ESCAPE, TAB→SDLK_TAB, ENTER→SDLK_RETURN, SPACE→SDLK_SPACE
  - [x] ASCII keyboard events generate SDLK values for letters/numbers
- [x] `pal_audio.h` + `pal_audio.cpp` - minimp3 streaming task + PCM FX cache
  - [x] Music path uses `configGetPath(fname)` for correct version directory
  - [x] Volume set to 100% via `bsp_audio_set_volume(100.0)`
- [x] `pal_font.h` + `pal_font.cpp` - Hershey vector font replacing SDL_ttf

### Shared Files (`main/shared/`)
- [x] `globals.h` - TANMATSU_BUILD flags, fixed 800x480, SD paths, PAL includes
- [x] `osdef.h` / `osdef.cpp` - stripped of SDL/Win32, MKDIR + BS_strdup
- [x] `bsscreen.h` / `bsscreen.cpp` - thin shim to pal_screen
- [x] `errorhandler.h` / `errorhandler.cpp` - ESP_LOGE based
- [x] `fonthandler.h` / `fonthandler.cpp` - thin shim to pal_font
- [x] `drawprimitives.h` / `drawprimitives.cpp` - BS_Surface wrappers, image loaders
- [x] `config.cpp` - SD card paths, `.extracted` marker to skip re-extraction on subsequent boots
- [x] `extractmetabmf.cpp` - uses fastopen, shows extraction status on screen, #ifdef DISABLE_BACKGROUND_ART guards preserved

### Image Loading (`main/shared/drawprimitives.cpp`)
- [x] BMP decoder: 8-bit (paletted), 24-bit and 32-bit uncompressed, top-down and bottom-up
- [x] PNG decoder: lodepng with custom PSRAM allocators (`lodepng_malloc/realloc/free`)
- [x] JPEG decoder: ESP32-P4 hardware JPEG engine via `esp_driver_jpeg` (lazy init, BGR888 → RGBA32)
  - [x] Manual PSRAM fallback when `jpeg_alloc_decoder_mem` fails for output buffer
- [x] Format detection by file extension (case-insensitive)
- [x] lodepng.h + lodepng.cpp copied to `main/pal/` as single-file library
- [x] CMakeLists.txt: added `esp_driver_jpeg` to PRIV_REQUIRES, `-DLODEPNG_NO_COMPILE_ALLOCATORS`

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
- [x] `main.cpp` - app_main, BSP init, game_task with 64KB PSRAM stack via xTaskCreateStaticPinnedToCore

## Build Status
- **Clean build as of 2026-03-12**: ~0xc8f70 bytes (823KB), 61% of partition free

## Known Issues

### JPEG hardware decoder limitations
- ESP32-P4 `jpeg_decoder_get_info` only parses SOF0 (baseline JPEG); progressive JPEGs return 0x0 dimensions
- Converted all 64 progressive JPEGs in blinkensisters source to baseline — still failing on device
- **Workaround**: Background art disabled (`DISABLE_BACKGROUND_ART=1`) to unblock other testing
- **TODO**: Investigate further — may need software JPEG fallback (stb_image.h or esp_jpeg component)

### Missing sprite files
- `sister_movenone.bmp`, `sister_moveleftup.bmp`, etc. not present in 24c3 addon — player invisible when idle
- May need fallback to `sister_moveleft.bmp` or addon-specific fix

## Next Steps
- [ ] Test keyboard input (Escape/Tab/Enter via scancodes)
- [ ] Test sprite transparency fix (alpha=0 pixels skipped)
- [ ] Step 7: Level loading & rendering — tiles, player physics, collision, monsters, Lua triggers
- [ ] Step 8: Audio — music streaming, sound FX on events
- [ ] Step 9: HUD & menu fonts verification
- [ ] Step 10: Polish, optimize FPS, memory profiling, fix JPEG decoder for backgrounds
