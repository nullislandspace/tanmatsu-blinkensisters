# BlinkenSisters Tanmatsu Port - Progress

## Status: Beta test candidate (2026-09-06)

Plays end to end: menu, addon selection, all LostPixels levels, death and
end screens, highscores, return to launcher. ~25 fps in gameplay. Six addons
published to the app repository as `at.cavac.blinkensisters` (`make apprepo`);
a seventh, `mz_xmas2007`, is built and in `sdcard/` but held back.

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

## Tools

Host-side, built with the local compiler (`make -C tools`), documented in
`tools/README.md`:

- `bmfextract` -- list, unpack, or `cat` one member of a `.bmf` archive
- `bmfcompress` -- build one from a config file; taken unchanged from
  upstream, so archives built here are byte-identical to the originals
- `mkicons.sh` -- regenerate `metadata/icon*.png` from the player sprite

Both C++ tools link `main/bmf/bmfconvert.*`, the same code the game uses, so
there is one implementation of the format rather than two that can drift.

Together they close the loop on shipped data without needing the upstream
asset tree, since everything is already inside the archives: extract, fix,
recompress, `make installbmf`. The upstream configs are preserved in
`bmfsource/` (see its README), and `mz_template` is a ready-made skeleton for
a new addon.

Worth knowing: `bmfcompress` **skips a missing source file with a warning**
rather than failing. An archive can quietly come out short, which is exactly
what happened to `fx_menu.mp3` (below).

---

## What the hardware JPEG decoder will not do

Six of the shipped backgrounds failed to load, and the two causes are both
places where the ESP32-P4 decoder is stricter or stranger than it looks.
Anyone porting image loading to this chip will hit them.

**The pixel count must be a multiple of 8.** `jpeg_parse_sof_marker()` rejects
a picture on `(width * height) % 8`, logging "Picture sizes not divisible by 8
are not supported". Note it is the *product*, not either dimension, so
perfectly ordinary sizes fail: 1341x900, 1475x661, 2431x530, 495x598 and
794x1123 are all art we ship. `jpeg_decoder_get_info()` does **not** apply the
rule, so the header parses fine and only `jpeg_decoder_process()` fails.

The loader works around it by rewriting the width and height in the SOF0
header of its own in-memory copy, shrinking the picture by the smallest amount
that satisfies the rule. This is safe only while the smaller size still spans
the same number of MCUs -- the scan is one stream of MCUs, and changing the
count desynchronises the decode into garbage -- so the search holds
`ceil(w/mcu_w)` and `ceil(h/mcu_h)` fixed and refuses rather than guessing if
nothing fits. Checked against libjpeg on the host for every background here:
the kept pixels come back identical except in the final row and column, where
chroma upsampling replicates a different edge sample and moves a channel by at
most 4/255. One to three pixels come off the right and bottom.

**MCU size comes from the sampling factors, not from `sample_method`.** The
driver's own `jpeg_parse_sof_marker()` computes `mcux = hi * 8`, `mcuy = vi *
8` from the first component. Deriving it from the `sample_method` enum that
`jpeg_decoder_get_info()` reports agrees for ordinary files but not for a
single-component picture that still carries 2x2 sampling factors --
`JPEG_DOWN_SAMPLING_GRAY` suggests an 8x8 MCU while the driver uses 16x16.
LostPixels' level6.jpg (3328x952, one component, 2x2) is exactly that: the
output is padded to 3328x960, the loader expected 3328x952, and the size check
threw the decode away. The loader now parses the SOF0 itself and uses the
sampling factors, the same rule the driver uses.

A wrong guess here is not always loud, either -- an earlier version of this
loader assumed 16x16 for everything, which produced a *sheared* picture rather
than an error whenever the real MCU was smaller.

---

## Known issues

### Player sprites that were never drawn
The engine asks for `sister_movenone`, `sister_moveup`, `sister_movedown` and
the four diagonals, logs "Can't load FgObjGFX ... ignored" for each, and
leaves the player invisible when idle.

These are not missing data. `bmfextract` and a search of the upstream tree
both say only `sister_moveleft.bmp` and `sister_moveright.bmp` have ever
existed, in any archive or any addon. This is artwork to create, not data to
recover. The cheap alternative is falling back to the last-facing sprite when
idle, which needs no artwork at all.

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

`make apprepo` copies exactly the assets `metadata.json` declares, rather than
globbing `sdcard/`, so that one file decides what ships: an archive can sit in
`sdcard/` unpublished (`mz_xmas2007` does) and removing its entry withdraws it.
Anything in the repository that metadata no longer names is reported as a
stray, since it would bloat the repository while shipping to nobody.

`make apprepo` stages the whole release into the app repository: metadata,
icons, the binary **renamed to application.bin** (the build calls it
tanmatsu-blinkensisters.bin, metadata.json names the other), and the BMFs
with their `addons/` subdirectory preserved, because that is what the asset
`source_file` entries say. It then re-reads metadata.json and checks every
declared asset actually landed -- with several assets across two directories a
silent `cp` omission would otherwise only surface as a failed install on
someone else's badge.

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
- Menu clicks were silent because nothing shipped `fx_menu.mp3`. The
  LostPixels config asks for `SND/fx_collect_pixel.mp3 -> fx_menu.mp3`, but
  that source is not in the addon tree (the sound lives in basedata), so
  bmfcompress skipped the line and no archive ever contained the file. The
  game now falls back to `fx_collect_pixel.mp3`, which is what the line
  intended. Confirmed working on device.
- Six backgrounds never loaded, killing 24c3 level 1 and LostPixels levels 3,
  6, 9, 13 and 20 with a fatal image error. Two separate hardware-decoder
  quirks, both described above.
- `mz_xmas2007` was missing. It is in the upstream `ADDONS` list with complete
  artwork (889 files, seven levels, three carols) but had never been built for
  this port; rebuilt with `bmfcompress` and added, taking the game to seven
  addons and `sdcard/` to 79 MB.
- The app icons are the player sprite (frame 0 of `sister_moveright.bmp`),
  green keyed to white. 32 is 1:1 and 64 a nearest-neighbour 2x so both stay
  crisp; only 16 is resampled, with a box filter -- point drops too many
  pixels to stay legible and Lanczos rings.
