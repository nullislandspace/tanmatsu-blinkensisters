#!/usr/bin/env python3
"""Rebuild a .bmf archive from its own (possibly edited) contents.

    tools/bmfrepack.py IN.bmf OUT.bmf [--tree DIR] [--rename OLD=NEW ...]
                       [--drop PATTERN ...]

Takes the record order -- directories, files, addon and music registrations --
from IN, and the file contents from DIR (a `bmfextract extract` of IN, edited
as needed; without --tree, IN is extracted to a temporary directory, which
only makes sense together with --rename or to check the round trip).
--rename OLD=NEW stores the member OLD under the name NEW, reading its
contents from DIR/NEW, and may be given many times. --drop PATTERN leaves out
every member whose name matches the shell-style pattern.

The archive is written by bmfcompress, so an unedited repack is byte-identical
to its input. Every member must exist in DIR: bmfcompress would silently skip
a missing file, which is how archives came out short before.
"""

import argparse
import fnmatch
import os
import subprocess
import sys
import tempfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
BMFEXTRACT = os.path.join(TOOLS, "bmfextract")
BMFCOMPRESS = os.path.join(TOOLS, "bmfcompress")

RECORDS = {"dir": "DIR", "reg-addon": "REGISTERADDON",
           "reg-music": "REGISTERMUSIC", "reg-poscap": "REGISTERPOSCAP"}


def die(msg):
    print("bmfrepack: " + msg, file=sys.stderr)
    sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--tree", help="directory holding the member files")
    ap.add_argument("--rename", action="append", default=[], metavar="OLD=NEW")
    ap.add_argument("--drop", action="append", default=[], metavar="PATTERN")
    args = ap.parse_args()

    renames = {}
    for r in args.rename:
        old, sep, new = r.partition("=")
        if not sep or not old or not new:
            die("--rename wants OLD=NEW, got '%s'" % r)
        renames[old] = new

    if not (os.access(BMFEXTRACT, os.X_OK) and os.access(BMFCOMPRESS, os.X_OK)):
        subprocess.run(["make", "-C", TOOLS], check=True, stdout=subprocess.DEVNULL)

    listing = subprocess.run([BMFEXTRACT, "list", args.input], check=True,
                             text=True, stdout=subprocess.PIPE).stdout.splitlines()

    with tempfile.TemporaryDirectory() as tmp:
        tree = args.tree
        if not tree:
            tree = os.path.join(tmp, "tree")
            subprocess.run([BMFEXTRACT, "extract", args.input, tree], check=True,
                           stdout=subprocess.DEVNULL)
        tree = os.path.abspath(tree)

        config = []
        for line in listing[1:]:
            if not line.startswith("  "):
                continue                      # the trailing summary line
            kind, _, rest = line.strip().partition(" ")
            rest = rest.strip()
            if kind == "file":
                name = rest.split(None, 1)[1]
                if any(fnmatch.fnmatchcase(name, pat) for pat in args.drop):
                    if name in renames:
                        die("%s is both dropped and renamed" % name)
                    continue
                name = renames.pop(name, name)
                src = os.path.join(tree, name)
                if not os.path.isfile(src):
                    die("%s is missing from %s" % (name, tree))
                if "#" in src or "#" in name or "|" in src:
                    die("bmfcompress cannot express the path '%s'" % name)
                config.append("FILE=%s|%s" % (src, name))
            elif kind in RECORDS:
                if "#" in rest:
                    die("bmfcompress would truncate '%s' at the '#'" % rest)
                config.append("%s=%s" % (RECORDS[kind], rest))
            else:
                die("cannot repack a '%s' record" % kind)
        if renames:
            die("--rename names members that are not in the archive: %s"
                % ", ".join(sorted(renames)))

        cfgpath = os.path.join(tmp, "config")
        with open(cfgpath, "w") as f:
            f.write("\n".join(config) + "\n")
        out = subprocess.run([BMFCOMPRESS, "META", cfgpath, os.path.abspath(args.output)],
                             text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if out.returncode != 0 or "WARNING" in out.stdout:
            die("bmfcompress failed:\n" + out.stdout)


if __name__ == "__main__":
    main()
