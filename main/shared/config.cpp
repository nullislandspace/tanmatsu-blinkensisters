#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "globals.h"
#include "showloading.h"
#include "extractmetabmf.h"
#include "errorhandler.h"
#include "fastopen.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"

static const char* CTAG = "config";

bool startupComplete = false;
char currentAddonName[PATH_MAX] = "";
static char file[PATH_MAX];

// On Tanmatsu, home dir is always /sd/blinkensisters
char* gethomepath() {
    return (char*)TANMATSU_WORK_DIR;
}

static bool file_exists(const char* path) {
    FILE* fh = fastopen(path, "r");
    if (!fh) return false;
    fastclose(fh);
    return true;
}

// configGetPath: resolve a game asset to a path in the working dir.
//
// With an addon selected, look in the addon's directory first, then fall back
// to the base game data, then to the assets shipped with the app. The base
// fallback is what lets an addon use the shared artwork it does not override:
// livelost.jpg, gameover.jpg and friends only exist in basedata.bmf, so
// without it dying inside any addon looked for a file that is never there and
// took the fatal-error path.
char* configGetPath(const char* fname) {
    if (strlen(fname) == 0) {
        if (strlen(currentAddonName) > 0) {
            snprintf(file, sizeof(file), "%s/V%s/ADDON/%s/%s",
                     TANMATSU_WORK_DIR, VERSION, currentAddonName, fname);
        } else {
            snprintf(file, sizeof(file), "%s/V%s/%s",
                     TANMATSU_WORK_DIR, VERSION, fname);
        }
        return file;
    }

    char candidate[PATH_MAX];

    if (strlen(currentAddonName) > 0) {
        snprintf(file, sizeof(file), "%s/V%s/ADDON/%s/%s",
                 TANMATSU_WORK_DIR, VERSION, currentAddonName, fname);
        if (file_exists(file)) {
            return file;
        }
        // Fall back to the base game data.
        snprintf(candidate, sizeof(candidate), "%s/V%s/%s",
                 TANMATSU_WORK_DIR, VERSION, fname);
    } else {
        snprintf(file, sizeof(file), "%s/V%s/%s",
                 TANMATSU_WORK_DIR, VERSION, fname);
        if (file_exists(file)) {
            return file;
        }
        snprintf(candidate, sizeof(candidate), "%s%s", RESPATH, fname);
    }

    if (file_exists(candidate)) {
        strncpy(file, candidate, sizeof(file) - 1);
        file[sizeof(file) - 1] = '\0';
        return file;
    }

    // Last resort: the assets shipped with the app.
    snprintf(candidate, sizeof(candidate), "%s%s", RESPATH, fname);
    if (file_exists(candidate)) {
        strncpy(file, candidate, sizeof(file) - 1);
        file[sizeof(file) - 1] = '\0';
    }
    // Nothing matched; `file` still holds the primary location, which is the
    // most useful thing to name in the caller's error message.
    return file;
}

char* getrespath(const char* filename) {
    char* result = (char*)malloc(PATH_MAX);
    if (!result) return NULL;
    snprintf(result, PATH_MAX, "%s%s", RESPATH, filename);
    return result;
}

void configSetBasePath(const char* path) {
    (void)path;
}

void configSetAddOn(const char* addOnName) {
    strncpy(currentAddonName, addOnName, sizeof(currentAddonName) - 1);
    currentAddonName[sizeof(currentAddonName) - 1] = '\0';
    printf("addon: %s\n", addOnName);
    if (startupComplete) {
        deInitShowLoading();
        initShowLoading("loading.jpg");
        showLoading();
    }
}

void configStartupComplete() {
    startupComplete = true;
}

// ---- Extraction stamps ------------------------------------------------------
//
// Each archive gets its own stamp file next to the data it unpacked,
// V<version>/.extracted_<archive>, recording the size, modification time and
// CRC32 of the archive it was unpacked from. An archive is unpacked again only
// when it no longer matches its stamp, so adding or replacing one addon costs
// that addon alone rather than all ~70 MB.
//
// Checking is tiered to keep a normal launch free of reading the archives:
//   - size differs                       -> changed, unpack
//   - size and mtime match               -> unchanged
//   - size matches, mtime differs        -> CRC the file; unpack only if that
//                                           differs too (a re-upload of the
//                                           same file just refreshes the stamp)
// The mtime shortcut is skipped when the clock that wrote the file was never
// set (a pre-2020 timestamp): every file then carries the same time, and a fix
// that keeps the length, say a one-character script change, would go unseen.

#define STAMP_MIN_TRUSTED_MTIME 1577836800   // 2020-01-01

typedef struct {
    long long size;
    long long mtime;
    uint32_t  crc;
} bmf_stamp_t;

static void stampPathFor(const char* bmfpath, char* out, size_t outlen) {
    // Keyed by file name alone: the same archive may sit in either directory
    // (see configFindArchive), and it is the same unpacked data either way.
    const char* base = strrchr(bmfpath, '/');
    base = base ? base + 1 : bmfpath;
    snprintf(out, outlen, "%s/V%s/.extracted_%s", TANMATSU_WORK_DIR, VERSION, base);
}

static bool stampRead(const char* path, bmf_stamp_t* st) {
    FILE* fh = fastopen(path, "r");
    if (!fh) return false;
    unsigned long crc = 0;
    int n = fscanf(fh, "size=%lld mtime=%lld crc32=%lx", &st->size, &st->mtime, &crc);
    fastclose(fh);
    st->crc = (uint32_t)crc;
    return n == 3;
}

static void stampWrite(const char* path, const bmf_stamp_t* st) {
    FILE* fh = fastopen(path, "w");
    if (!fh) {
        ESP_LOGW(CTAG, "Cannot write %s; the archive will be unpacked again next launch", path);
        return;
    }
    fprintf(fh, "size=%lld\nmtime=%lld\ncrc32=%08lx\n",
            st->size, st->mtime, (unsigned long)st->crc);
    fastclose(fh);
}

static bool fileCrc32(const char* path, uint32_t* out) {
    FILE* fh = fastopen(path, "rb");
    if (!fh) return false;
    const size_t bufsize = 64 * 1024;
    uint8_t* buf = (uint8_t*)heap_caps_malloc(bufsize, MALLOC_CAP_SPIRAM);
    if (!buf) {
        fastclose(fh);
        return false;
    }
    uint32_t crc = 0;
    size_t n;
    while ((n = fread(buf, 1, bufsize, fh)) > 0) {
        crc = esp_rom_crc32_le(crc, buf, n);
    }
    bool ok = !ferror(fh);
    heap_caps_free(buf);
    fastclose(fh);
    *out = crc;
    return ok;
}

// Unpack bmfpath unless its stamp says this exact file was unpacked already.
// Returns true when it was unpacked.
static bool configExtractIfChanged(const char* bmfpath, bool force) {
    struct stat fst;
    if (stat(bmfpath, &fst) != 0) {
        ESP_LOGW(CTAG, "%s: not found", bmfpath);
        return false;
    }

    char stamppath[PATH_MAX];
    stampPathFor(bmfpath, stamppath, sizeof(stamppath));

    bmf_stamp_t now = { (long long)fst.st_size, (long long)fst.st_mtime, 0 };
    bool haveCrc = false;
    bmf_stamp_t old;

    if (force) {
        ESP_LOGI(CTAG, "%s: forced update", bmfpath);
    } else if (!stampRead(stamppath, &old)) {
        ESP_LOGI(CTAG, "%s: not unpacked yet", bmfpath);
    } else if (old.size != now.size) {
        ESP_LOGI(CTAG, "%s: size changed (%lld -> %lld)", bmfpath, old.size, now.size);
    } else if (old.mtime == now.mtime && now.mtime >= STAMP_MIN_TRUSTED_MTIME) {
        ESP_LOGI(CTAG, "%s: unchanged", bmfpath);
        return false;
    } else {
        ESP_LOGI(CTAG, "%s: %s, comparing contents", bmfpath,
                 now.mtime >= STAMP_MIN_TRUSTED_MTIME ? "timestamp changed"
                                                      : "clock was not set");
        haveCrc = fileCrc32(bmfpath, &now.crc);
        if (haveCrc && now.crc == old.crc) {
            ESP_LOGI(CTAG, "%s: unchanged (crc32 %08lx)", bmfpath, (unsigned long)now.crc);
            if (old.mtime != now.mtime) stampWrite(stamppath, &now);
            return false;
        }
        ESP_LOGI(CTAG, "%s: contents changed", bmfpath);
    }

    // Drop the stamp first, so an unpack cut short (power, a crash) is not
    // mistaken for a finished one on the next launch.
    unlink(stamppath);
    char path[PATH_MAX];
    strncpy(path, bmfpath, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    if (!extractMetaBMF(path, true)) {
        return false;
    }
    if (!haveCrc && !fileCrc32(bmfpath, &now.crc)) {
        ESP_LOGW(CTAG, "%s: cannot read it back for its crc32", bmfpath);
        return true;
    }
    stampWrite(stamppath, &now);
    return true;
}

static bool isArchiveName(const char* name) {
    size_t n = strlen(name);
    return n > 4 && strcasecmp(name + n - 4, ".bmf") == 0;
}

bool configFindArchive(const char* file, char* out, size_t outlen, bool* downloaded) {
    snprintf(out, outlen, "%s/%s", TANMATSU_ARCHIVE_DIR, file);
    if (file_exists(out)) {
        if (downloaded) *downloaded = true;
        return true;
    }
    if (strcasecmp(file, "basedata.bmf") == 0) {
        snprintf(out, outlen, "%s/%s", TANMATSU_APP_PATH, file);
    } else {
        snprintf(out, outlen, "%s/addons/%s", TANMATSU_APP_PATH, file);
    }
    if (downloaded) *downloaded = false;
    return file_exists(out);
}

bool configExtractArchive(const char* bmfpath, bool force) {
    if (!configExtractIfChanged(bmfpath, force)) {
        return false;
    }
    const char* base = strrchr(bmfpath, '/');
    base = base ? base + 1 : bmfpath;
    if (strcasecmp(base, "basedata.bmf") != 0) {
        char name[PATH_MAX], tmp[PATH_MAX];
        snprintf(name, sizeof(name), "%s", base);
        name[strlen(name) - 4] = '\0';   // drop ".bmf"
        snprintf(tmp, sizeof(tmp), "ADDON/%s/config", name);
        registerLevelconfig(configGetPath(tmp));
    }
    return true;
}

void configForgetArchive(const char* file) {
    char stamppath[PATH_MAX];
    stampPathFor(file, stamppath, sizeof(stamppath));
    unlink(stamppath);
}

bool configAddonInstalled(const char* dir) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/V%s/ADDON/%s/level1.conf", TANMATSU_WORK_DIR, VERSION, dir);
    return file_exists(path);
}

bool configBaseDataInstalled() {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/V%s/menubg.jpg", TANMATSU_WORK_DIR, VERSION);
    return file_exists(path);
}

// Addon archives come from two places: downloaded ones in TANMATSU_ARCHIVE_DIR,
// and any the launcher (or `make installbmf`) put in the app's own directory.
// A downloaded copy wins over one of the same name there, so unpack only one
// of each name -- they would otherwise take turns overwriting each other's
// data and stamp on every launch.
static void configExtractAddons(bool force) {
    const char* dirs[2] = { TANMATSU_ARCHIVE_DIR, TANMATSU_APP_PATH "/addons" };
    for (int d = 0; d < 2; d++) {
        DIR* dir = opendir(dirs[d]);
        if (!dir) continue;
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            if (!isArchiveName(ent->d_name) || strcasecmp(ent->d_name, "basedata.bmf") == 0) continue;
            char path[PATH_MAX];
            if (d == 1) {
                snprintf(path, sizeof(path), "%s/%s", TANMATSU_ARCHIVE_DIR, ent->d_name);
                if (file_exists(path)) continue;   // the downloaded copy was used
            }
            snprintf(path, sizeof(path), "%s/%s", dirs[d], ent->d_name);
            configExtractArchive(path, force);
        }
        closedir(dir);
    }
}

void configInit(const bool forceUpdate) {
    // Create working directory structure
    mkdir(TANMATSU_WORK_DIR, S_IRWXU);
    char verdir[PATH_MAX];
    snprintf(verdir, sizeof(verdir), "%s/V%s", TANMATSU_WORK_DIR, VERSION);
    mkdir(verdir, S_IRWXU);
    char addondir[PATH_MAX];
    snprintf(addondir, sizeof(addondir), "%s/V%s/ADDON", TANMATSU_WORK_DIR, VERSION);
    mkdir(addondir, S_IRWXU);

    // The single all-or-nothing marker the stamps replaced. Nothing reads it
    // any more; remove it so it does not suggest otherwise.
    char legacymarker[PATH_MAX];
    snprintf(legacymarker, sizeof(legacymarker), "%s/.extracted", verdir);
    unlink(legacymarker);

    mkdir(TANMATSU_ARCHIVE_DIR, S_IRWXU);

    // Base data may not be there at all: the app ships without archives and
    // downloads them on first start (see addonstore).
    char basedatapath[PATH_MAX];
    if (configFindArchive("basedata.bmf", basedatapath, sizeof(basedatapath), NULL)) {
        configExtractIfChanged(basedatapath, forceUpdate);
    }

    configExtractAddons(forceUpdate);
}
