// BlinkenSisters - Tanmatsu port
//
// In-game downloads; see addonstore.h.
//
// Where things live:
//   index       ADDON_INDEX_URL (raw file in the GitHub repository)
//   archives    TANMATSU_ARCHIVE_DIR/<file>, next to <file>.version holding
//               the version installed and <file>.part while downloading
//   unpacked    the usual /sd/blinkensisters/V<version>/, via configExtractArchive
//
// Archives the launcher put in the app's own directory (older installs, or
// `make installbmf` during development) still count as installed; a download
// of the same name replaces them.

#include "globals.h"
#include "addonstore.h"
#include "bsgui.h"
#include "bsscreen.h"
#include "blending.h"
#include "drawprimitives.h"
#include "errorhandler.h"
#include "fonthandler.h"
#include "joystick.h"
#include "sound.h"
#include "progressui.h"
#include "pal/pal_net.h"
#include "fastopen.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "addonstore";

#define INDEX_MAX_BYTES     (256 * 1024)
#define CONNECT_TIMEOUT_MS  45000

typedef struct {
	char id[64];
	char name[100];
	char desc[160];
	char file[100];
	char url[512];
	char sha256[65];
	char minver[16];
	Uint32 version;
	uint64_t size;
} STORE_ENTRY;

typedef struct {
	STORE_ENTRY base;
	STORE_ENTRY* addons;
	int count;
} STORE_INDEX;

typedef enum {
	STATUS_MISSING = 0,
	STATUS_CURRENT,
	STATUS_UPDATE,
	STATUS_NEEDS_NEWER_GAME,
} STORE_STATUS;

// ---- Small UI helpers -------------------------------------------------------

static bool anyKeyReleased() {
	bool key = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYUP) key = true;
	}
	if (getJoystickReleases() != JOYSTICK_NONE) key = true;
	return key;
}

static void drawPanel(const char* title, const char* text, const char* footer) {
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
	blend_brightenRect(40, 60, SCR_WIDTH - 80, SCR_HEIGHT - 120, 0x00181818);
	drawrect(40, 60, SCR_WIDTH - 80, 2, 0x0000cc00);
	drawrect(40, SCR_HEIGHT - 62, SCR_WIDTH - 80, 2, 0x0000cc00);
	renderFontHandlerText(0, 90, title, BS_Color_WHITE, true, false, FONT_menufont_30);
	renderFontHandlerText(70, 160, text, BS_Color_WHITE, false, false, FONT_textfont_20);
	if (footer) {
		renderFontHandlerText(0, SCR_HEIGHT - 110, footer, MENUCOLOR_INACTIVE, true, false, FONT_textfont_20);
	}
	BS_Flip(gScreen);
}

static void messageScreen(const char* title, const char* text) {
	ESP_LOGI(TAG, "%s: %s", title, text);
	drawPanel(title, text, "Press any key");
	flushJoystick();
	while (!anyKeyReleased()) {
		SDL_Delay(50);
	}
	soundPlayFX(FX_MENU);
}

// ---- WiFi and the index -----------------------------------------------------

static bool waitForNetwork() {
	PAL_NetStartConnect();
	Uint32 start = SDL_GetTicks();
	Uint32 lastDraw = 0;
	flushJoystick();
	for (;;) {
		PAL_NetState state = PAL_NetGetState();
		switch (state) {
			case PAL_NET_CONNECTED:
				return true;
			case PAL_NET_NO_NETWORKS:
				messageScreen("No WiFi network",
				              "No WiFi network is set up on this badge.\n\n"
				              "Add one in the launcher's WiFi settings,\nthen try again.");
				return false;
			case PAL_NET_FAILED:
				messageScreen("No connection",
				              "Could not connect to any of the WiFi\nnetworks saved in the launcher.\n\n"
				              "Check that one is in range and try again.");
				return false;
			case PAL_NET_UNAVAILABLE:
				messageScreen("No WiFi", "The WiFi radio did not start.\n\nRestart the badge and try again.");
				return false;
			default:
				break;
		}
		Uint32 now = SDL_GetTicks();
		if (now - start > CONNECT_TIMEOUT_MS) {
			messageScreen("No connection", "Connecting to WiFi is taking too long.\n\nTry again in a moment.");
			return false;
		}
		if (anyKeyReleased()) {
			return false;
		}
		if (now - lastDraw > 250) {
			lastDraw = now;
			static const char* const dots[] = { "", ".", "..", "..." };
			char text[64];
			snprintf(text, sizeof(text), "Connecting to WiFi%s", dots[(now / 400) % 4]);
			drawPanel("Please wait", text, "Press any key to cancel");
		}
		SDL_Delay(20);
	}
}

static void copyString(char* dst, size_t len, cJSON* obj, const char* key) {
	cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
	snprintf(dst, len, "%s", cJSON_IsString(item) ? item->valuestring : "");
}

static bool parseEntry(cJSON* obj, STORE_ENTRY* e) {
	memset(e, 0, sizeof(*e));
	if (!cJSON_IsObject(obj)) return false;
	copyString(e->id, sizeof(e->id), obj, "id");
	copyString(e->name, sizeof(e->name), obj, "name");
	copyString(e->desc, sizeof(e->desc), obj, "description");
	copyString(e->file, sizeof(e->file), obj, "file");
	copyString(e->url, sizeof(e->url), obj, "url");
	copyString(e->sha256, sizeof(e->sha256), obj, "sha256");
	copyString(e->minver, sizeof(e->minver), obj, "min_game_version");
	cJSON* version = cJSON_GetObjectItemCaseSensitive(obj, "version");
	cJSON* size = cJSON_GetObjectItemCaseSensitive(obj, "size");
	e->version = cJSON_IsNumber(version) ? (Uint32)version->valuedouble : 0;
	e->size = cJSON_IsNumber(size) ? (uint64_t)size->valuedouble : 0;
	// The file name becomes a path on the SD card: keep it a plain name.
	return e->id[0] && e->file[0] && e->url[0] && strlen(e->sha256) == 64 && e->size > 0 &&
	       !strchr(e->file, '/') && !strchr(e->file, '\\') && e->file[0] != '.';
}

static void freeIndex(STORE_INDEX* idx) {
	heap_caps_free(idx->addons);
	idx->addons = NULL;
	idx->count = 0;
}

// Fetch and parse the index, telling the player if that fails.
static bool fetchIndex(STORE_INDEX* idx) {
	memset(idx, 0, sizeof(*idx));
	drawPanel("Please wait", "Loading the addon list", NULL);

	char* body = NULL;
	size_t len = 0;
	char err[160] = "";
	if (!PAL_NetFetch(ADDON_INDEX_URL, &body, &len, INDEX_MAX_BYTES, err, sizeof(err))) {
		char text[256];
		snprintf(text, sizeof(text), "Could not load the addon list:\n%s", err);
		messageScreen("Download failed", text);
		return false;
	}
	cJSON* root = cJSON_ParseWithLength(body, len);
	free(body);
	cJSON* format = root ? cJSON_GetObjectItemCaseSensitive(root, "format") : NULL;
	if (!cJSON_IsNumber(format)) {
		cJSON_Delete(root);
		messageScreen("Download failed", "The addon list could not be read.");
		return false;
	}
	if (format->valueint != 1) {
		cJSON_Delete(root);
		messageScreen("Update needed",
		              "The addon list needs a newer version\nof BlinkenSisters. Update the game\nin the launcher.");
		return false;
	}

	cJSON* addons = cJSON_GetObjectItemCaseSensitive(root, "addons");
	int n = cJSON_IsArray(addons) ? cJSON_GetArraySize(addons) : 0;
	idx->addons = (STORE_ENTRY*)heap_caps_calloc(n > 0 ? n : 1, sizeof(STORE_ENTRY), MALLOC_CAP_SPIRAM);
	bool baseOk = parseEntry(cJSON_GetObjectItemCaseSensitive(root, "basedata"), &idx->base);
	if (idx->addons) {
		cJSON* item;
		cJSON_ArrayForEach(item, addons) {
			if (parseEntry(item, &idx->addons[idx->count])) {
				idx->count++;
			} else {
				ESP_LOGW(TAG, "skipping a malformed index entry");
			}
		}
	}
	cJSON_Delete(root);
	if (!baseOk || !idx->addons) {
		freeIndex(idx);
		messageScreen("Download failed", "The addon list could not be read.");
		return false;
	}
	ESP_LOGI(TAG, "index: base data v%u, %d addon(s)", (unsigned)idx->base.version, idx->count);
	return true;
}

// ---- Installed state --------------------------------------------------------

static int compareVersions(const char* a, const char* b) {
	int va[3] = {0, 0, 0}, vb[3] = {0, 0, 0};
	sscanf(a, "%d.%d.%d", &va[0], &va[1], &va[2]);
	sscanf(b, "%d.%d.%d", &vb[0], &vb[1], &vb[2]);
	for (int i = 0; i < 3; i++) {
		if (va[i] != vb[i]) return va[i] < vb[i] ? -1 : 1;
	}
	return 0;
}

static void recordPath(const STORE_ENTRY* e, char* out, size_t len) {
	snprintf(out, len, "%s/%s.version", TANMATSU_ARCHIVE_DIR, e->file);
}

static Uint32 installedVersion(const STORE_ENTRY* e) {
	char path[300];
	recordPath(e, path, sizeof(path));
	FILE* fh = fastopen(path, "r");
	if (!fh) return 0;
	unsigned v = 0;
	if (fscanf(fh, "version=%u", &v) != 1) v = 0;
	fastclose(fh);
	return v;
}

static void writeRecord(const STORE_ENTRY* e) {
	char path[300];
	recordPath(e, path, sizeof(path));
	FILE* fh = fastopen(path, "w");
	if (!fh) {
		ESP_LOGW(TAG, "cannot write %s", path);
		return;
	}
	fprintf(fh, "version=%u\nsha256=%s\n", (unsigned)e->version, e->sha256);
	fastclose(fh);
}

static STORE_STATUS entryStatus(const STORE_ENTRY* e, bool* downloaded) {
	char path[300];
	bool dl = false;
	STORE_STATUS status;
	if (!configFindArchive(e->file, path, sizeof(path), &dl)) {
		status = STATUS_MISSING;
	} else if (dl) {
		status = installedVersion(e) >= e->version ? STATUS_CURRENT : STATUS_UPDATE;
	} else {
		// Put there by the launcher: no record, so go by size.
		struct stat st;
		status = (stat(path, &st) == 0 && (uint64_t)st.st_size == e->size) ? STATUS_CURRENT : STATUS_UPDATE;
	}
	if (downloaded) *downloaded = dl;
	if (status != STATUS_CURRENT && e->minver[0] && compareVersions(e->minver, VERSION) > 0) {
		status = STATUS_NEEDS_NEWER_GAME;
	}
	return status;
}

// ---- Install and remove -----------------------------------------------------

typedef struct {
	const char* name;
	Uint32 startTick;
	uint64_t startBytes;
	bool started;
} DOWNLOAD_CTX;

static bool downloadProgress(void* ctxp, uint64_t done, uint64_t total) {
	DOWNLOAD_CTX* ctx = (DOWNLOAD_CTX*)ctxp;
	Uint32 now = SDL_GetTicks();
	if (!ctx->started) {
		ctx->started = true;
		ctx->startTick = now;
		ctx->startBytes = done;
	}
	char have[24], all[24], line1[160], line2[160];
	progressUIFormatBytes(done, have, sizeof(have));
	progressUIFormatBytes(total, all, sizeof(all));
	snprintf(line1, sizeof(line1), "%s: %s of %s", ctx->name, have, all);

	Uint32 elapsed = now - ctx->startTick;
	if (elapsed > 2000 && done > ctx->startBytes) {
		double rate = (double)(done - ctx->startBytes) * 1000.0 / elapsed;   // bytes per second
		Uint32 left = (Uint32)((double)(total - done) / rate);
		char speed[24];
		progressUIFormatBytes((uint64_t)rate, speed, sizeof(speed));
		snprintf(line2, sizeof(line2), "%s/s, about %u:%02u left - any key stops", speed,
		         (unsigned)(left / 60), (unsigned)(left % 60));
	} else {
		snprintf(line2, sizeof(line2), "Press any key to stop");
	}
	progressUIDraw(done, total, line1, line2, false);
	return !anyKeyReleased();
}

// Download (or resume) one archive and unpack it. Tells the player how it went
// when it did not work.
static bool installEntry(const STORE_ENTRY* e) {
	char dest[300];
	snprintf(dest, sizeof(dest), "%s/%s", TANMATSU_ARCHIVE_DIR, e->file);
	ESP_LOGI(TAG, "installing %s v%u from %s", e->id, (unsigned)e->version, e->url);

	DOWNLOAD_CTX ctx = { e->name, 0, 0, false };
	char err[200] = "";
	flushJoystick();
	progressUIBegin(PROGRESSUI_DOWNLOAD);
	bool ok = PAL_NetDownload(e->url, dest, e->size, e->sha256, downloadProgress, &ctx, err, sizeof(err));
	progressUIEnd();
	if (!ok) {
		if (strcmp(err, "Stopped") == 0) {
			messageScreen("Download stopped",
			              "Choose it again to continue\nwhere it stopped.");
		} else {
			char text[300];
			snprintf(text, sizeof(text), "%s\n\nChoose it again to retry; it continues\nwhere it stopped.", err);
			messageScreen("Download failed", text);
		}
		return false;
	}
	writeRecord(e);
	configExtractArchive(dest, true);
	ESP_LOGI(TAG, "installed %s v%u", e->id, (unsigned)e->version);
	return true;
}

static void removeTree(const char* path) {
	DIR* dir = opendir(path);
	if (dir) {
		struct dirent* ent;
		while ((ent = readdir(dir)) != NULL) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
			char child[512];
			snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
			struct stat st;
			if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
				removeTree(child);
			} else {
				unlink(child);
			}
		}
		closedir(dir);
	}
	rmdir(path);
}

// Drop an addon's line ("name|description|dir") from addons.dat.
static void unregisterAddon(const char* dir) {
	char path[300];
	snprintf(path, sizeof(path), "%s", configGetPath("addons.dat"));
	FILE* fh = fastopen(path, "r");
	if (!fh) return;
	char* kept = (char*)heap_caps_calloc(1, 64 * 1024, MALLOC_CAP_SPIRAM);
	if (!kept) {
		fastclose(fh);
		return;
	}
	size_t used = 0;
	char line[1000];
	while (fgets(line, sizeof(line), fh)) {
		char copy[1000];
		snprintf(copy, sizeof(copy), "%s", line);
		char* tmp;
		while ((tmp = strchr(copy, '\n'))) *tmp = 0;
		while ((tmp = strchr(copy, '\r'))) *tmp = 0;
		char* last = strrchr(copy, '|');
		if (copy[0] && last && strcasecmp(last + 1, dir) == 0) continue;
		size_t n = strlen(line);
		if (used + n < 64 * 1024) {
			memcpy(kept + used, line, n);
			used += n;
		}
	}
	fastclose(fh);
	fh = fastopen(path, "w");
	if (fh) {
		fwrite(kept, 1, used, fh);
		fastclose(fh);
	}
	heap_caps_free(kept);
}

static void removeEntry(const STORE_ENTRY* e) {
	char path[512];
	snprintf(path, sizeof(path), "%s/%s", TANMATSU_ARCHIVE_DIR, e->file);
	unlink(path);
	snprintf(path, sizeof(path), "%s/%s.part", TANMATSU_ARCHIVE_DIR, e->file);
	unlink(path);
	recordPath(e, path, sizeof(path));
	unlink(path);
	configForgetArchive(e->file);
	snprintf(path, sizeof(path), "%s/V%s/ADDON/%s", TANMATSU_WORK_DIR, VERSION, e->id);
	removeTree(path);
	unregisterAddon(e->id);
	ESP_LOGI(TAG, "removed %s", e->id);
}

// ---- Entry points -----------------------------------------------------------

bool addonStoreEnsureBaseData() {
	while (!configBaseDataInstalled()) {
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
		if (!guiYesNoDialog("BlinkenSisters needs to download", "its game data (about 2 MB). Go ahead?", true)) {
			return false;
		}
		if (!waitForNetwork()) continue;
		STORE_INDEX idx;
		if (!fetchIndex(&idx)) continue;
		installEntry(&idx.base);
		freeIndex(&idx);
	}
	return true;
}

void addonStoreOfferLostPixels() {
	if (configAddonInstalled("LostPixels")) return;
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
	if (!guiYesNoDialog("Lost Pixels, the main game, is not", "installed yet. Download it now?", true)) {
		return;
	}
	if (!waitForNetwork()) return;
	STORE_INDEX idx;
	if (!fetchIndex(&idx)) return;
	for (int i = 0; i < idx.count; i++) {
		if (strcasecmp(idx.addons[i].id, "LostPixels") == 0) {
			installEntry(&idx.addons[i]);
			break;
		}
	}
	freeIndex(&idx);
}

static void statusText(const STORE_ENTRY* e, char* out, size_t len) {
	char size[24];
	progressUIFormatBytes(e->size, size, sizeof(size));
	bool downloaded = false;
	switch (entryStatus(e, &downloaded)) {
		case STATUS_MISSING:
			snprintf(out, len, "Not installed - %s", size);
			break;
		case STATUS_CURRENT:
			snprintf(out, len, downloaded ? "Installed (version %u) - select to remove"
			                              : "Installed (version %u)", (unsigned)e->version);
			break;
		case STATUS_UPDATE:
			snprintf(out, len, "Update available - %s", size);
			break;
		case STATUS_NEEDS_NEWER_GAME:
			snprintf(out, len, "Needs BlinkenSisters %s or newer", e->minver);
			break;
	}
}

static void chooseEntry(const STORE_ENTRY* e) {
	char size[24], line[200];
	progressUIFormatBytes(e->size, size, sizeof(size));
	bool downloaded = false;
	switch (entryStatus(e, &downloaded)) {
		case STATUS_MISSING:
		case STATUS_UPDATE:
			snprintf(line, sizeof(line), "Download %s?", e->name);
			if (guiYesNoDialog(line, size, true)) {
				installEntry(e);
			}
			break;
		case STATUS_CURRENT:
			if (!downloaded) {
				snprintf(line, sizeof(line), "%s is up to date. It sits in the app's\nown folder, so the launcher manages it.", e->name);
				messageScreen(e->name, line);
			} else {
				snprintf(line, sizeof(line), "Remove %s?", e->name);
				char freed[64];
				snprintf(freed, sizeof(freed), "This frees %s on the SD card.", size);
				if (guiYesNoDialog(line, freed, false)) {
					removeEntry(e);
				}
			}
			break;
		case STATUS_NEEDS_NEWER_GAME:
			snprintf(line, sizeof(line), "%s needs BlinkenSisters %s or newer.\n\nUpdate the game in the launcher.",
			         e->name, e->minver);
			messageScreen("Update needed", line);
			break;
	}
	flushJoystick();
}

void addonStoreMenu() {
	if (!waitForNetwork()) return;
	STORE_INDEX idx;
	if (!fetchIndex(&idx)) return;

	if (entryStatus(&idx.base, NULL) == STATUS_UPDATE) {
		char size[24];
		progressUIFormatBytes(idx.base.size, size, sizeof(size));
		char line[64];
		snprintf(line, sizeof(line), "Download it now (%s)?", size);
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
		if (guiYesNoDialog("New game data is available.", line, true)) {
			installEntry(&idx.base);
		}
	}

	SDL_Surface* bg = BS_IMG_Load_Fullscreen(configGetPath("menuonlinebg.jpg"), IGNORE_FILE_ERROR);
	// Entries 1..count are addons, count+1 is "Back" (1-based, as the other menus).
	Uint32 max = (Uint32)idx.count + 1;
	Uint32 sel = 1;
	Uint32 offs = 1;
	bool running = true;
	flushJoystick();

	while (running) {
		if (bg) {
			SDL_BlitSurface(bg, NULL, gScreen, NULL);
		} else {
			drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
		}
		while (((Sint32)sel - (Sint32)offs) > 3) offs++;
		while (((Sint32)sel - (Sint32)offs) < 0) offs--;

		for (Uint32 i = offs; i <= SDL_min(max, offs + 3); i++) {
			int y = 70 + (i - offs) * 90;
			SDL_Color color = (i == sel) ? MENUCOLOR_ACTIVE : MENUCOLOR_INACTIVE;
			if (i == max) {
				renderFontHandlerText(10, y + 10, "Back", color, true, false, FONT_menufont_50);
				continue;
			}
			const STORE_ENTRY* e = &idx.addons[i - 1];
			blend_darkenRect(20, y, 760, 80, 0x00a0a0a0);
			Uint32 frame = (i == sel) ? 0xd0d0d0 : 0x606060;
			drawrect(20, y, 760, 1, frame);
			drawrect(20, y, 1, 80, frame);
			drawrect(20, y + 79, 760, 1, frame);
			drawrect(779, y, 1, 80, frame);
			renderFontHandlerText(33, y + 3, e->name, color, false, false, FONT_menufont_50);
			char status[160];
			statusText(e, status, sizeof(status));
			renderFontHandlerText(33, y + 57, status, color, false, false, FONT_textfont_20);
		}
		BS_Flip(gScreen);

		Uint32 joymove = getJoystickReleases();
		if (joymove & JOYSTICK_UP) {
			soundPlayFX(FX_MENU);
			sel = (sel > 1) ? sel - 1 : max;
		} else if (joymove & JOYSTICK_DOWN) {
			soundPlayFX(FX_MENU);
			sel = (sel < max) ? sel + 1 : 1;
		} else if (joymove & JOYSTICK_ACTION) {
			soundPlayFX(FX_MENU);
			if (sel == max) {
				running = false;
			} else {
				chooseEntry(&idx.addons[sel - 1]);
			}
		}

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type != SDL_KEYUP) continue;
			CHECK_BOSSKEY;
			if (event.key.keysym.sym == SDLK_w) {
				soundPlayFX(FX_MENU);
				sel = (sel > 1) ? sel - 1 : max;
			} else if (event.key.keysym.sym == SDLK_s) {
				soundPlayFX(FX_MENU);
				sel = (sel < max) ? sel + 1 : 1;
			}
		}
		SDL_Delay(50);
	}

	SDL_FreeSurface(bg);
	freeIndex(&idx);
}
