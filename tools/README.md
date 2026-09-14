# BMF tools

Host-side utilities for the `.bmf` archives the game keeps its data in, and
for publishing them. They build with the local compiler and run on your PC,
not on the badge.

    make -C tools

## bmfextract

Read an archive.

    ./tools/bmfextract list    sdcard/basedata.bmf
    ./tools/bmfextract extract sdcard/addons/LostPixels.bmf /tmp/lp
    ./tools/bmfextract cat     sdcard/basedata.bmf livelost.jpg > livelost.jpg

`list` prints a line per member with its record type, size and name, plus the
`DIR` and `REGISTER*` records that tell the game where to unpack things and
which addons and music tracks to register. `extract` unpacks everything,
creating subdirectories as the archive asks for them.

## bmfcompress

Build an archive from a config file. Taken unchanged from the upstream
project, so archives built here are byte-identical to the originals.

    ./tools/bmfcompress META bmfsource/basedata_config sdcard/basedata.bmf
    cd <addon source dir> && bmfcompress META config out.bmf

The config format is `COMMAND=value`, one per line, `#` starts a comment:

| Command | Meaning |
|---|---|
| `DIR=path` | create `path` under the game's data directory on unpack |
| `FILE=source\|target` | add `source` from disk, unpacking it as `target` |
| `REGISTERADDON=name\|desc\|dir` | list this addon in the game's addon menu |
| `REGISTERMUSIC=name\|file\|desc` | register a track with the music player |
| `REGISTERPOSCAP=...` | register a position capture (attract mode) |

A missing `FILE=` source is skipped with a warning rather than failing the
build, which is worth knowing: an archive can quietly come out short.

## bmfrepack.py

Rebuild an archive from its own, edited, contents, keeping its record order
(directories, files, registrations). An unedited repack is byte-identical.

    ./tools/bmfextract extract sdcard/addons/icy.bmf /tmp/icy
    # ... edit files in /tmp/icy ...
    ./tools/bmfrepack.py sdcard/addons/icy.bmf /tmp/icy.bmf --tree /tmp/icy \
        --rename ADDON/Icy/old.gif=ADDON/Icy/old.png --drop 'ADDON/Icy/unused_*'

`--rename OLD=NEW` stores a member under a new name (contents from the tree at
NEW); `--drop PATTERN` leaves out matching members. Unlike bmfcompress it
refuses to leave out a member silently. This is how the wormhole GIFs became
PNGs and how `mz_xmas2007` lost two thirds of its animation frames (see
`bmfsource/README.md`).

## publish-addons.py

Publishes the game data for in-game download and maintains
`addons/index.json`, the list the game reads to find it: the base data
(`sdcard/basedata.bmf`, always published, since the app ships without it) and
the addons. Each version is its own GitHub release, tagged
`basedata-v<version>` or `addon-<id>-v<version>` and never replaced, so a URL
in any index ever published keeps pointing at the same bytes.

    make publishaddons                    # dry run: print what would happen
    make publishaddons PUBLISH=1          # do it
    make publishaddons PUBLISH=1 ADDONARGS="--add sdcard/addons/icy.bmf"
    make publishaddons PUBLISH=1 ADDONARGS="--remove MZ_Pnog"

The usual loop is: change an archive, commit and push it, `make
publishaddons PUBLISH=1`. The script SHA-256s every archive in the index; only
the ones that changed get a version bump and a release, then the index is
committed and pushed. Name, description and id come from the archive's own
`REGISTERADDON` record, so there is nothing to type in.

It refuses to run with uncommitted archives or index, or with the branch out
of sync with origin, and creates every release and checks GitHub's SHA-256 of
each upload before it touches the index. A run that fails partway can be
repeated; an upload that arrived corrupt stops it with the command to delete
that release. Addon releases are marked not-latest, so they never displace a
game release as the repository's "Latest". Removing an addon only drops its
index entry; its releases stay, for badges still holding an older index.

Index layout: `format` (1), `basedata` (one entry) and `addons` (a list).
Entry fields: `id` (the addon directory, or `basedata`), `name`,
`description`, `file`, `version`, `min_game_version` (the game version current
when the entry was added, unless `--min-game-version` says otherwise; the game
refuses entries that need a newer version), `sha256`, `size`, `url`.

## Artwork the hardware can decode

Backgrounds and screens are JPEG, decoded by the ESP32-P4's JPEG unit, which
is fussier than a software decoder. If you are drawing a level, keep to:

- **Baseline JPEG only.** Progressive is rejected outright — there is no
  software fallback. (`file yourimage.jpg` says which; re-save with
  `convert in.jpg -interlace none out.jpg`.)
- **8-bit samples, 1 or 3 components.**
- Any size. The engine works around the decoder's "pixel count must be a
  multiple of 8" rule by trimming one to three pixels off the right and bottom
  edges, and logs a warning saying so — but a size where `width * height` is
  already a multiple of 8 avoids the trim entirely.

PNG goes through lodepng in software and has none of these constraints.

## Editing shipped data

The tools close the loop without needing the upstream asset tree, since
everything is already inside the archive:

    ./tools/bmfextract extract sdcard/addons/24c3.bmf /tmp/24c3
    # ... fix something in /tmp/24c3 ...
    ./tools/bmfrepack.py sdcard/addons/24c3.bmf /tmp/24c3.bmf --tree /tmp/24c3
    cp /tmp/24c3.bmf sdcard/addons/24c3.bmf
    make installbmf BMF=24c3          # try it on the badge
    git commit ... && git push && make publishaddons PUBLISH=1   # ship it

The original configs are preserved in `bmfsource/` for reference — see
`bmfsource/README.md`.

**After changing data** there is nothing to do on the device. The game stamps
each archive it unpacks with its size, time and CRC32, and unpacks again any
archive that no longer matches (details in `PROGRESS.md`, "Installing").
`make resetdata` forces a full unpack if one is ever needed.
