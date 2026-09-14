#!/usr/bin/env python3
"""Publish addon archives as GitHub releases and keep addons/index.json in step.

addons/index.json is both the list of what is published and what the game
reads to find downloads. Each entry points at an immutable release asset:

    tag  addon-<id>-v<version>      e.g. addon-LostPixels-v2
    url  https://github.com/<repo>/releases/download/<tag>/<file>

A run compares every archive in the index against sdcard/addons/. An archive
whose SHA-256 differs from its entry gets the next version number, a new
release, and an updated entry; unchanged archives are left alone, so fixing
one small addon never re-uploads the others. Releases are never overwritten
or deleted: badges holding an older index still download a consistent file.

Without --publish nothing is changed anywhere; the plan is printed. --add and
--remove are part of the plan too, so repeat them with --publish to apply them.

    tools/publish-addons.py                         # dry run: what would happen
    tools/publish-addons.py --publish               # create releases, commit + push index
    tools/publish-addons.py --add sdcard/addons/icy.bmf       # start publishing one
    tools/publish-addons.py --remove mz_xmas2007    # stop listing one (releases stay)

Order of operations on --publish: all releases are created and their uploads
checked first, and only then is the index committed and pushed, so the
published index never names a file that is not there. A run that dies halfway
can simply be repeated: a release that already exists with the right file is
reused, one whose file is missing gets it uploaded, and one holding a
different file stops the run.
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INDEX = os.path.join(ROOT, "addons", "index.json")
ADDON_DIR = os.path.join(ROOT, "sdcard", "addons")
BMFEXTRACT = os.path.join(ROOT, "tools", "bmfextract")
GLOBALS_H = os.path.join(ROOT, "main", "shared", "globals.h")
DEFAULT_REPO = "nullislandspace/tanmatsu-blinkensisters"
INDEX_FORMAT = 1


def die(msg):
    print("error: " + msg, file=sys.stderr)
    sys.exit(1)


def run(cmd, check=True, capture=True):
    r = subprocess.run(cmd, cwd=ROOT, text=True,
                       stdout=subprocess.PIPE if capture else None,
                       stderr=subprocess.PIPE if capture else None)
    if check and r.returncode != 0:
        die("%s failed:\n%s" % (" ".join(cmd[:3]), (r.stderr or "").strip()))
    return r


def game_version():
    with open(GLOBALS_H) as f:
        m = re.search(r'^#define VERSION "(.*)"', f.read(), re.M)
    if not m:
        die("cannot find VERSION in " + GLOBALS_H)
    return m.group(1)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def registration(path):
    """The REGISTERADDON record inside the archive: (name, description, id)."""
    if not os.access(BMFEXTRACT, os.X_OK):
        run(["make", "-C", "tools"])
    out = run([BMFEXTRACT, "list", path]).stdout
    regs = [l.split("reg-addon", 1)[1].strip() for l in out.splitlines() if "reg-addon" in l]
    if len(regs) != 1:
        die("%s: expected exactly one addon registration, found %d" % (path, len(regs)))
    parts = regs[0].split("|")
    if len(parts) != 3 or not parts[2]:
        die("%s: malformed registration '%s'" % (path, regs[0]))
    if not re.fullmatch(r"[A-Za-z0-9_.-]+", parts[2]):
        die("%s: addon id '%s' is not usable in a tag name" % (path, parts[2]))
    return parts[0], parts[1], parts[2]


def load_index():
    if not os.path.exists(INDEX):
        return {"format": INDEX_FORMAT, "addons": []}
    with open(INDEX) as f:
        idx = json.load(f)
    if idx.get("format") != INDEX_FORMAT:
        die("%s: unsupported format %r" % (INDEX, idx.get("format")))
    return idx


def save_index(idx):
    idx["addons"].sort(key=lambda e: (e["id"] != "LostPixels", e["id"].lower()))
    os.makedirs(os.path.dirname(INDEX), exist_ok=True)
    with open(INDEX, "w") as f:
        json.dump(idx, f, indent=2)
        f.write("\n")


def tag_for(entry_id, version):
    return "addon-%s-v%d" % (entry_id, version)


def release_assets(repo, tag):
    """Assets of the release at tag as {name: sha256 hex}, or None if there is
    no such release. GitHub computes the digest itself on upload, so matching
    it proves the file arrived intact, not just at the right length."""
    r = run(["gh", "api", "repos/%s/releases/tags/%s" % (repo, tag)], check=False)
    if r.returncode != 0:
        if "Not Found" in (r.stdout or "") + (r.stderr or ""):
            return None
        die("cannot query release %s: %s" % (tag, (r.stderr or r.stdout).strip()))
    assets = {}
    for a in json.loads(r.stdout)["assets"]:
        digest = a.get("digest") or ""
        if not digest.startswith("sha256:"):
            die("release %s: GitHub reports no sha256 digest for %s" % (tag, a["name"]))
        assets[a["name"]] = digest[len("sha256:"):]
    return assets


def check_git_ready():
    rel = os.path.relpath(INDEX, ROOT)
    if run(["git", "status", "--porcelain", "--", rel]).stdout.strip():
        die("%s has uncommitted changes; commit or discard them first" % rel)
    run(["git", "fetch", "--quiet", "origin"])
    branch = run(["git", "rev-parse", "--abbrev-ref", "HEAD"]).stdout.strip()
    counts = run(["git", "rev-list", "--left-right", "--count",
                  "HEAD...origin/%s" % branch]).stdout.split()
    if counts != ["0", "0"]:
        die("branch %s is not in sync with origin (ahead %s, behind %s); "
            "push or pull first" % (branch, counts[0], counts[1]))
    return branch


def check_archive_committed(path):
    rel = os.path.relpath(path, ROOT)
    if not run(["git", "ls-files", "--", rel]).stdout.strip():
        die("%s is not committed; a release must match a committed archive" % rel)
    if run(["git", "status", "--porcelain", "--", rel]).stdout.strip():
        die("%s has uncommitted changes; commit it first" % rel)


def main():
    sys.stdout.reconfigure(line_buffering=True)
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--publish", action="store_true",
                    help="actually create releases and commit + push the index")
    ap.add_argument("--add", action="append", default=[], metavar="BMF",
                    help="start publishing this archive (under sdcard/addons/)")
    ap.add_argument("--remove", action="append", default=[], metavar="ID",
                    help="drop this addon from the index; its releases are kept")
    ap.add_argument("--min-game-version", metavar="VER",
                    help="minimum game version for addons added or changed in this run "
                         "(default: kept for changed addons, current game version for new ones)")
    ap.add_argument("--repo", default=DEFAULT_REPO)
    args = ap.parse_args()

    idx = load_index()
    by_id = {e["id"]: e for e in idx["addons"]}
    plan = []      # (entry, action, archive path)
    changed_index = False

    for rid in args.remove:
        if rid not in by_id:
            die("--remove %s: not in the index" % rid)
        idx["addons"].remove(by_id.pop(rid))
        print("remove    %-16s (its releases stay available)" % rid)
        changed_index = True

    for path in args.add:
        path = os.path.abspath(path)
        if os.path.dirname(path) != ADDON_DIR:
            die("--add %s: archives must live in sdcard/addons/" % path)
        if not os.path.exists(path):
            die("--add %s: no such file" % path)
        name, desc, rid = registration(path)
        if rid in by_id:
            die("--add %s: addon %s is already in the index" % (path, rid))
        entry = {"id": rid, "name": name, "description": desc,
                 "file": os.path.basename(path), "version": 0,
                 "min_game_version": args.min_game_version or game_version()}
        idx["addons"].append(entry)
        by_id[rid] = entry

    for entry in idx["addons"]:
        path = os.path.join(ADDON_DIR, entry["file"])
        if not os.path.exists(path):
            die("%s: %s is missing (use --remove %s to stop publishing it)"
                % (entry["id"], path, entry["id"]))
        name, desc, rid = registration(path)
        if rid != entry["id"]:
            die("%s: the archive now registers itself as '%s'" % (entry["file"], rid))
        digest = sha256_of(path)
        size = os.path.getsize(path)
        if digest == entry.get("sha256") and name == entry["name"] and desc == entry["description"]:
            print("unchanged %-16s v%d" % (rid, entry["version"]))
            plan.append((entry, "verify", path))
            continue
        new = dict(entry)
        new.update(name=name, description=desc, sha256=digest, size=size)
        if digest != entry.get("sha256"):
            new["version"] = entry["version"] + 1
            if args.min_game_version:
                new["min_game_version"] = args.min_game_version
            new["url"] = "https://github.com/%s/releases/download/%s/%s" % (
                args.repo, tag_for(rid, new["version"]), entry["file"])
            print("%-9s %-16s v%d -> v%d  (%.1f MB)" % (
                "new" if entry["version"] == 0 else "changed", rid,
                entry["version"], new["version"], size / 1e6))
            plan.append((new, "release", path))
        else:
            print("renamed   %-16s v%d  (name or description only, no new release)"
                  % (rid, entry["version"]))
            plan.append((new, "verify", path))
        idx["addons"][idx["addons"].index(entry)] = new
        changed_index = True

    releases = [p for p in plan if p[1] == "release"]
    if not args.publish:
        print("\nDry run: %d release(s) to create, index %s. Run with --publish to do it."
              % (len(releases), "would change" if changed_index else "unchanged"))
        return

    if run(["gh", "auth", "status"], check=False).returncode != 0:
        die("gh is not logged in (gh auth login)")
    branch = check_git_ready()
    head = run(["git", "rev-parse", "HEAD"]).stdout.strip()

    for entry, action, path in plan:
        tag = tag_for(entry["id"], entry["version"])
        check_archive_committed(path)
        assets = release_assets(args.repo, tag)
        size = os.path.getsize(path)
        if action == "verify":
            if assets is None or assets.get(entry["file"]) != entry["sha256"]:
                die("%s: release %s is missing or does not hold this %s. "
                    "Not repairing a published version automatically."
                    % (entry["id"], tag, entry["file"]))
            continue
        if assets is None:
            print("creating release %s ..." % tag)
            run(["gh", "release", "create", tag, path, "--repo", args.repo,
                 "--target", head, "--latest=false",
                 "--title", "Addon: %s v%d" % (entry["name"], entry["version"]),
                 "--notes", "%s\n\nFile: `%s`, %d bytes\nSHA-256: `%s`\n\n"
                            "Published by tools/publish-addons.py for the Tanmatsu "
                            "BlinkenSisters addon downloader." % (
                                entry["description"], entry["file"], size, entry["sha256"])],
                capture=False)
        elif entry["file"] not in assets:
            print("release %s exists without its file; uploading ..." % tag)
            run(["gh", "release", "upload", tag, path, "--repo", args.repo], capture=False)
        elif assets[entry["file"]] != entry["sha256"]:
            die("release %s already holds a different %s. Published versions are "
                "never replaced automatically; if this release is a botched upload "
                "nobody can have used yet, delete it with\n"
                "    gh release delete %s --cleanup-tag --repo %s\nand run again"
                % (tag, entry["file"], tag, args.repo))
        else:
            print("release %s already holds this file; reusing it" % tag)
        assets = release_assets(args.repo, tag)
        if not assets or assets.get(entry["file"]) != entry["sha256"]:
            die("release %s: upload of %s did not arrive intact" % (tag, entry["file"]))

    if not changed_index:
        print("Index unchanged; nothing to commit.")
        return

    save_index(idx)
    rel = os.path.relpath(INDEX, ROOT)
    summary = ", ".join("%s v%d" % (e["id"], e["version"]) for e, a, _ in releases)
    if args.remove:
        summary = ", ".join(filter(None, [summary, "removed " + ", ".join(args.remove)]))
    run(["git", "add", "--", rel])
    run(["git", "commit", "-q", "-m", "Publish addons: %s" % (summary or "index metadata"),
         "--", rel])
    run(["git", "push", "-q", "origin", branch])
    print("Index committed and pushed: %s" % summary)


if __name__ == "__main__":
    main()
