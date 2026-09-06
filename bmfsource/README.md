# BMF source configs

The config files the shipped `.bmf` archives were built from, preserved from
the upstream project (`/home/cavac/src/blinkensisters`) so the data can be
rebuilt or fixed here. See `tools/README.md` for the format and the tools.

    bmfsource/basedata_config      -> sdcard/basedata.bmf
    bmfsource/addons/<name>/config -> sdcard/addons/<name>.bmf

Only the configs are kept, not the artwork: for everything the game ships, the
assets are already inside the archives and `bmfextract` gets them back out.

Two configs here have **no shipped archive**, so their artwork is not
recoverable from this repository and has to come from the upstream tree:
`mz_template` (the skeleton for a new addon, and still the best starting point
for a custom level) and `mz_testlevel` (a development test level). Upstream
builds nine addons; this port ships seven.

## Rebuilding an addon

With the upstream asset tree available:

    cd /home/cavac/src/blinkensisters/ADDONS/mz_xmas2007
    <port>/tools/bmfcompress META config <port>/sdcard/addons/mz_xmas2007.bmf

That is exactly how `mz_xmas2007.bmf` was produced — it was in the upstream
`ADDONS` list but had never been built for this port. Check the result with
`bmfextract list`: the member count should equal the number of `FILE=` lines
in the config, since bmfcompress does not fail on a missing source.

## Known discrepancies between config and shipped archive

These were found by comparing the configs against `bmfextract list` output,
and are worth knowing before trusting a config to reproduce an archive.

**`fx_menu.mp3` is missing.** `addons/LostPixels/config` line 14 says
`FILE=SND/fx_collect_pixel.mp3|fx_menu.mp3`, but there is no
`SND/fx_collect_pixel.mp3` in the LostPixels source tree — the sound lives in
basedata instead. `bmfcompress` skips a missing source with a warning, so the
line silently produced nothing and no archive contains `fx_menu.mp3`. The game
now falls back to `fx_collect_pixel.mp3`, which is what the line intended
anyway; rebuilding with a corrected source path would fix it properly.

**Only two player sprites exist.** `sister_moveleft.bmp` and
`sister_moveright.bmp`, in every addon. The engine also asks for
`sister_movenone`, `sister_moveup`, `sister_movedown` and the four diagonals,
logs "Can't load FgObjGFX ... ignored" for each, and leaves the player
invisible when idle. These were never drawn — not in any archive and not in
the upstream tree — so this is artwork to create, not data to recover.
