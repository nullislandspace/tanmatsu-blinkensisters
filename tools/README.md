# BMF tools

Host-side utilities for the `.bmf` archives the game ships its data in. They
build with the local compiler and run on your PC, not on the badge.

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

The two tools close the loop without needing the upstream asset tree, since
everything is already inside the archive:

    ./tools/bmfextract extract sdcard/addons/24c3.bmf /tmp/24c3
    # ... fix something in /tmp/24c3 ...
    # write a config listing the files, then:
    ./tools/bmfcompress META /tmp/24c3/config sdcard/addons/24c3.bmf
    make installbmf

The original configs are preserved in `bmfsource/` for reference — see
`bmfsource/README.md`.

**After changing data**, delete `/sd/blinkensisters/V<version>/.extracted` on
the device. The game unpacks each archive once and records that it has done
so; without removing the marker it will keep using what it unpacked before.
