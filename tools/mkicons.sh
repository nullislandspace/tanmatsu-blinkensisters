#!/bin/sh
# Regenerate metadata/icon*.png from the player sprite.
#
# The sprite sheet is 32x64: two 32x32 frames stacked, frame 0 on top. Frame 0
# is the idle pose -- upright and horizontally centred -- which reads better as
# an icon than frame 1's walk. Transparency in the game's artwork is pure green
# (see convertToBSSurface), and it becomes white here.
#
# Scaling is deliberate: 32 is the sprite 1:1 and 64 is a nearest-neighbour 2x,
# so both stay crisp pixel art. Only 16 is resampled, with a box filter --
# point drops too many pixels to stay legible at that size and Lanczos rings.
#
# Needs ImageMagick, and tools/bmfextract (make -C tools).
set -e
cd "$(dirname "$0")/.."

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

./tools/bmfextract cat sdcard/addons/LostPixels.bmf \
    ADDON/LostPixels/sister_moveright.bmp > "$TMP/sprite.bmp"

convert "$TMP/sprite.bmp" -crop 32x32+0+0 +repage \
    -fill white -opaque '#00FF00' "$TMP/base.png"

convert "$TMP/base.png" -background white -alpha remove -alpha off \
    -filter box -resize 16x16 PNG24:metadata/icon16.png
convert "$TMP/base.png" -background white -alpha remove -alpha off \
    PNG24:metadata/icon32.png
convert "$TMP/base.png" -background white -alpha remove -alpha off \
    -filter point -resize 64x64 PNG24:metadata/icon64.png

echo "Wrote metadata/icon16.png, icon32.png, icon64.png"
