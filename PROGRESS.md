# BlinkenSisters Tanmatsu Port - Progress

## Status: Beta test candidate (2026-09-06)

Plays end to end: menu, addon selection, all LostPixels levels, death and
end screens, highscores, return to launcher. ~25 fps in gameplay.

---

## Performance: what the hardware actually costs

This is the most transferable thing learned here, and it was all measured
rather than guessed. The frame profiler (`main/shared/profile.*`) prints a
per-phase table every three seconds; `DISABLE_FRAME_PROFILER` compiles it out.

**Measured constants on this hardware (ESP32-P4, 800x480 logical):**

| Thing | Cost |
|---|---|
| PPA throughput | ~20 ns per pixel + ~200 MB/s streaming |
| PSRAM via PPA DMA (32-bit copy) | ~124 MB/s |
| CPU rotation, tiled, 32-bit | ~48 MB/s (cache-hostile by nature) |
| `bsp_display_blit` | effectively free -- it is asynchronous |

**The cost is per-PIXEL, not per-byte.** Two full-screen PPA ops a frame is
768000 pixels, about 32 ms, and that is 75% of the frame no matter what the
pixel format is. Going 32-bit -> 16-bit took the frame 56 -> 42 ms, not
56 -> 28: the bytes halved but the pixel count did not.

**The journey, for reference:**

| Change | fps | frame |
|---|---|---|
| start (CPU rotate, 32-bit) | 10.8 | 93 ms |
| PPA rotate (32-bit) | 17.7 | 56 ms |
| RGB565 everywhere | 22.5 | 44 ms |
| native 565 HUD blends | ~25 | ~39 ms |

**Two hypotheses that were wrong, so nobody re-tests them:**

- *The pump task's scheduling latency.* Fitting `fixed + bytes/rate` to four
  measurements gave a suspiciously exact ~10.5 ms fixed cost per op, and
  `CONFIG_FREERTOS_HZ=100` is a 10 ms tick. Running ops synchronously in the
  caller (`PPA_DIRECT_BLOCKING`) moved it 33.9 -> 32.0 ms. Not it.
- *Dynamic frequency scaling.* `CONFIG_PM_ENABLE` with auto DFS drops APB to
  40 MHz when the CPU looks idle, and a frame spends 75% of its time blocked
  on the PPA, which looks exactly like idle. Holding CPU + APB frequency locks
  changed `ppa wait` by 0.2 ms. Not it either.

**What is left, both structural:**

- Overlap CPU with PPA (double-buffer `gScreen`): ~10 ms of CPU work hiding
  behind 32 ms of PPA, so roughly 30 fps. This is exactly the case the pump
  task was designed for and it still exists behind `PPA_DIRECT_BLOCKING`.
- Eliminate the full-screen background copy, leaving one PPA op instead of
  two: roughly 38 fps, but it means compositing in rotated space -- a real
  refactor of every drawing primitive.

---

## Architecture

### PAL layer (`main/pal/`)
- `pal_surface` -- BS_Surface, **RGB565**, cache-line aligned (the PPA needs it)
- `pal_screen` -- gScreen, panel framebuffer, the 90-degree flip
- `pal_ppa` -- PPA compositor, modelled on SynthEngine3D's `se_ppa`
- `pal_input` -- BSP events -> joystick state + release edges + SDL events
- `pal_audio` -- one mixer task owning I2S: music + 8 one-shot voices
- `pal_font` -- Hershey vector font replacing SDL_ttf
- `lodepng` -- PNG decoding, PSRAM allocators

### Pixel format
Surfaces are RGB565. **Colours in the game code are still 32-bit**
(`0xAABBGGRR`): blending, Lua, `drawrect` and SDL_Color all pass those
around, and `BS_GetPixel`/`BS_SetPixel` pack at the one boundary where
pixels are touched. Loaders pack once at load.

RGB565 has no alpha, but the blitter only ever used alpha as a binary skip
test, so nothing was lost: transparent pixels become `BS_TRANSPARENT` (a
reserved magenta the blitter skips), and loaders nudge any genuinely opaque
pixel that lands on that value by one part in 32. Colour keys are nudged
identically so they still match.

### PPA use
- **Flip**: RGB565 -> RGB565 with `ROTATION_ANGLE_270` (90 degrees clockwise).
- **Whole-surface blits and fills** only. A PPA write is only safe where it
  covers the entire destination, because handing the result back to the CPU
  means invalidating those cache lines and an invalidate discards any dirty
  line in the range. `BS_SurfaceFinalize` marks the static, fully opaque
  sources that may be read by DMA.
- Sprites, tiles and fonts stay on the CPU: they need the colour-key and
  transparency tests the PPA's block move does not do.

### Input
Every key arrives **twice**: once as a navigation event and once as a
scancode. Menus therefore act on the joystick *release edge* only, and the
SDL handler takes just W/S, which have no navigation twin. Acting on both
moved two entries per press; acting on the *press* left the release queued
for whatever screen the action opened, which then ate it as its own input.

SDL key events come from scancodes, not the ASCII stream -- the latter fires
on press only and auto-repeats.

---

## Known issues

### Missing sprites (data, not code)
`sister_movenone.bmp`, `sister_moveleftup.bmp`, `sister_moveleftdown.bmp`,
`sister_moverightup.bmp`, `sister_moverightdown.bmp`, `sister_moveup.bmp`,
`sister_movedown.bmp` are absent from `LostPixels.bmf`. The player is
invisible when idle. Only `sister_moveleft` and `sister_moveright` exist.

### No menu sound effect
`fx_menu.mp3` exists in neither `basedata.bmf` nor any addon, so `FX_MENU`
is silent. The other five predefined effects load and play.

### FORCE_PPA_ROTATE still on
`main/pal/pal_screen.cpp` uses the PPA flip even if `calibrate_flip()` cannot
reproduce the CPU reference. Colours are verified correct, but it is not
recorded whether that is because calibration now passes or because the switch
is covering a failure. The boot log says which: `PPA flip calibrated:` versus
`FORCE_PPA_ROTATE is on`.

### Large backgrounds are memory-hungry
LostPixels' biggest is 3328x952. Kept as loaded and wrapped at draw time (up
to four blits when the offset straddles an edge, one full-screen blit
otherwise), which is what made it fit -- the previous screen-padded copy plus
its immediate duplicate peaked near 57 MB on a 32 MB device.

---

## Installing

- `make install` -- application binary, metadata, icons. Fast.
- `make installbmf` -- the ~70 MB of game data. Only needed on a fresh device
  or after `sdcard/` changes.

The game unpacks the BMFs once and records it with a `.extracted` marker.
After changing the data, delete `/sd/blinkensisters/V<version>/.extracted` on
the device or it will keep using what it already unpacked.

`metadata.json` sets `external_only`, so the launcher installs to SD card
only -- the data does not fit in internal flash.

---

## Fixed this round (for the record)

- Launcher rejected the app: `"type": "esp32-app"` is not a type it knows
  (`appfs`, `elf`, `script`).
- Background art never appeared: `DISABLE_BACKGROUND_ART` was defined in
  *two* places and either one disables it.
- Hardware JPEG decode was sheared: the decoder pads output to whole MCUs and
  the MCU size follows chroma subsampling (16x16 at 4:2:0, **8x8 at 4:4:4**,
  16x8 at 4:2:2). The loader assumed 16x16 for everything.
- Death sound looped forever: the voice cursor was 16.16 fixed point in a
  `uint32_t`, which wraps after 65536 frames -- 1.49 s at 44100 Hz.
- Audio was two tasks writing one I2S channel with no mixer, and effects were
  written synchronously from the game task (up to a 200 ms stall each).
- The six built-in sound effects were never loaded at all.
- `exit(0)` from the quit cheat never returns from a FreeRTOS task.
- Assets were looked up in the addon directory and then the app's own, never
  the base game data in between -- so dying inside any addon hit a fatal error
  looking for `livelost.jpg`.
- `dieWithError` spun in a bare delay loop, so a fatal error was a dead end.
- The HUD used fixed columns spaced for the original TTF; Hershey glyphs are
  much wider and the fields overlapped. Laid out from measured widths now.
- Backdrops drawn for 4:3 are stretched to 800x480 on load.
