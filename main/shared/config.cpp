#include <string.h>
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

bool startupComplete = false;
char currentAddonName[PATH_MAX] = "";
static char file[PATH_MAX];

// On Tanmatsu, home dir is always /sd/blinkensisters
char* gethomepath() {
    return (char*)TANMATSU_WORK_DIR;
}

// configGetPath: build path in working dir
char* configGetPath(const char* fname) {
    FILE* filetest;
    if (strlen(currentAddonName) > 0) {
        snprintf(file, sizeof(file), "%s/V%s/ADDON/%s/%s",
                 TANMATSU_WORK_DIR, VERSION, currentAddonName, fname);
    } else {
        snprintf(file, sizeof(file), "%s/V%s/%s",
                 TANMATSU_WORK_DIR, VERSION, fname);
    }
    if (strlen(fname) == 0) return file;

    // Check if file exists; if not, check in RESPATH (app assets)
    filetest = fastopen(file, "r");
    if (filetest) {
        fastclose(filetest);
    } else {
        char respath_file[PATH_MAX];
        snprintf(respath_file, sizeof(respath_file), "%s%s", RESPATH, fname);
        filetest = fastopen(respath_file, "r");
        if (filetest) {
            fastclose(filetest);
            strncpy(file, respath_file, sizeof(file) - 1);
            file[sizeof(file) - 1] = '\0';
        }
    }
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

static void configExtractAddons() {
    // Extract BMF files from app path to working dir
    char addondirname[PATH_MAX];
    snprintf(addondirname, sizeof(addondirname), "%s/addons", TANMATSU_APP_PATH);
    DIR* dir = opendir(addondirname);
    if (!dir) {
        printf("No addons directory found at %s\n", addondirname);
        return;
    }
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        char* ext = strrchr(ent->d_name, '.');
        if (!ext || strcmp(ext, ".bmf") != 0) continue;
        char tmp[PATH_MAX], tmp2[PATH_MAX];
        snprintf(tmp, sizeof(tmp), "%s/%s", addondirname, ent->d_name);
        extractMetaBMF(tmp, true);
        strncpy(tmp2, ent->d_name, sizeof(tmp2) - 1);
        tmp2[sizeof(tmp2) - 1] = '\0';
        // Remove .bmf extension
        size_t nlen = strlen(tmp2);
        if (nlen > 4) tmp2[nlen - 4] = '\0';
        snprintf(tmp, sizeof(tmp), "ADDON/%s/config", tmp2);
        registerLevelconfig(configGetPath(tmp));
    }
    closedir(dir);
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

    // Check if extraction already done (marker file)
    char markerpath[PATH_MAX];
    snprintf(markerpath, sizeof(markerpath), "%s/.extracted", verdir);
    FILE* marker = fastopen(markerpath, "r");
    if (marker && !forceUpdate) {
        fastclose(marker);
        printf("Data already extracted, skipping BMF extraction\n");
        return;
    }
    if (marker) fastclose(marker);

    // Extract basedata
    char basedatapath[PATH_MAX];
    snprintf(basedatapath, sizeof(basedatapath), "%s/basedata.bmf", TANMATSU_APP_PATH);
    extractMetaBMF(basedatapath, true);

    // Extract addon BMFs
    configExtractAddons();

    // Write marker file
    marker = fastopen(markerpath, "w");
    if (marker) {
        fprintf(marker, "%s\n", VERSION);
        fastclose(marker);
    }
}
