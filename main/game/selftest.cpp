// BlinkenSisters - Tanmatsu port
//
// Self-test: load every level of every installed addon, run one physics tick
// and draw one frame -- which between them run the level's config, its
// scripts' init, physics and paint callbacks, and load all of its artwork --
// and log the outcome of each to the debug console.
//
// A level that fails must not end the run, so DIE() is made recoverable for
// the duration (see dieRecoveryPoint): it jumps back here, the level's
// half-built state is torn down, and the next level starts. A hard crash still
// ends it, but every level logs a "loading" line first, so the last one in the
// log names the culprit.
//
// Everything the test logs carries the "selftest" tag:
//   I selftest: [24c3] level 3/12: loading
//   W selftest: [24c3] level 3/12: OK, 2 warning/error log line(s): W (...) ...
//   E selftest: [24c3] level 4/12: FAILED in load: error 102: ...
// and a summary at the end repeats every level that was not clean.

#include "globals.h"
#include "selftest.h"
#include "engine.h"
#include "gameengine.h"
#include "levelhandler.h"
#include "fonthandler.h"
#include "errorhandler.h"
#include "joystick.h"
#include "sound.h"
#include "blending.h"
#include "bsscreen.h"
#include "bl_lua.h"
#include "bl_lua_objbindings.h"
#include "fastopen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "selftest";

#define SELFTEST_MAX_ADDONS  64
#define SELFTEST_MAX_LEVELS  999
#define SELFTEST_MAX_REPORTS 256

typedef enum {
	STAGE_LOAD = 0,
	STAGE_PHYSICS,
	STAGE_RENDER,
} SELFTEST_STAGE;

static const char* stageName[] = { "load", "physics", "render" };

typedef struct {
	char name[100];      // display name
	char dir[100];       // directory under ADDON/
} SELFTEST_ADDON;

typedef struct {
	char addon[100];
	Uint32 level;
	bool failed;
	int issues;
	char detail[256];
} SELFTEST_REPORT;

// ---- Counting warnings and errors ------------------------------------------
//
// A level can load and still be wrong -- an image that decodes only after
// being trimmed, a sound that is not there -- and those show up as W or E log
// lines rather than as a DIE(). Count them per level by sitting in front of
// the log output, and keep the first one this task wrote for the report.

static vprintf_like_t prevVprintf = NULL;
static int logIssues = 0;
static char firstIssue[256];
static TaskHandle_t testTask = NULL;

static void stripAnsiAndNewlines(char* s) {
	char* w = s;
	for (char* r = s; *r; r++) {
		if (*r == '\033') {
			while (*r && *r != 'm') r++;
			if (!*r) break;
			continue;
		}
		if (*r == '\n' || *r == '\r') continue;
		*w++ = *r;
	}
	*w = 0;
}

static int selfTestVprintf(const char* fmt, va_list ap) {
	const char* p = fmt;
	if (p[0] == '\033') {
		const char* m = strchr(p, 'm');
		if (m) p = m + 1;
	}
	if ((p[0] == 'W' || p[0] == 'E') && p[1] == ' ' && p[2] == '(') {
		logIssues++;
		if (!firstIssue[0] && xTaskGetCurrentTaskHandle() == testTask) {
			va_list copy;
			va_copy(copy, ap);
			vsnprintf(firstIssue, sizeof(firstIssue), fmt, copy);
			va_end(copy);
			stripAnsiAndNewlines(firstIssue);
		}
	}
	return prevVprintf(fmt, ap);
}

// ---- Helpers ----------------------------------------------------------------

static bool fileExists(const char* path) {
	FILE* fh = fastopen(path, "r");
	if (!fh) return false;
	fastclose(fh);
	return true;
}

// addons.dat: one "name|description|directory" per line.
static Uint32 readAddonList(SELFTEST_ADDON* addons, Uint32 maxAddons) {
	FILE* fh = fopen(configGetPath("addons.dat"), "r");
	if (!fh) {
		ESP_LOGE(TAG, "cannot open %s", configGetPath("addons.dat"));
		return 0;
	}
	Uint32 count = 0;
	char line[512];
	while (count < maxAddons && fgets(line, sizeof(line), fh)) {
		char* tmp;
		while ((tmp = strchr(line, '\n'))) *tmp = 0;
		while ((tmp = strchr(line, '\r'))) *tmp = 0;
		if (!line[0]) continue;
		char* desc = strchr(line, '|');
		char* dir = desc ? strchr(desc + 1, '|') : NULL;
		if (!dir) {
			ESP_LOGE(TAG, "addons.dat: malformed line '%s'", line);
			continue;
		}
		*desc = 0;
		snprintf(addons[count].name, sizeof(addons[count].name), "%s", line);
		snprintf(addons[count].dir, sizeof(addons[count].dir), "%s", dir + 1);
		count++;
	}
	fclose(fh);
	return count;
}

// Levels are numbered from 1 without gaps; the game finds the last one the
// same way (see playGame).
static Uint32 countLevels() {
	Uint32 n = 0;
	char fname[32];
	while (n < SELFTEST_MAX_LEVELS) {
		snprintf(fname, sizeof(fname), "level%u.conf", (unsigned)(n + 1));
		if (!fileExists(configGetPath(fname))) break;
		n++;
	}
	return n;
}

static bool stopRequested() {
	bool stop = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYUP) stop = true;
	}
	if (getJoystickReleases() != JOYSTICK_NONE) stop = true;
	return stop;
}

static void drawStatus(const char* line1, const char* line2, bool failed) {
	blend_darkenRect(0, SCR_HEIGHT - 90, SCR_WIDTH, 90, 0x00c0c0c0);
	renderFontHandlerText(10, SCR_HEIGHT - 85, line1,
	                      failed ? BS_Color_RED : BS_Color_WHITE, false, false, FONT_textfont_20);
	renderFontHandlerText(10, SCR_HEIGHT - 58, line2, BS_Color_WHITE, false, false, FONT_textfont_12);
	renderFontHandlerText(10, SCR_HEIGHT - 30, "Self-test running -- press any key to stop",
	                      BS_Color_WHITE, false, false, FONT_textfont_12);
	BS_Flip(gScreen);
}

// ---- One level --------------------------------------------------------------

// Static rather than local: it is written between setjmp() and a possible
// longjmp() back to it, after which non-volatile locals are indeterminate.
static SELFTEST_STAGE stage;
static jmp_buf levelJmp;

// Returns true if the level ran; on failure `detail` holds the error.
static bool runOneLevel(char* detail, size_t detailLen) {
	stage = STAGE_LOAD;
	bool ok;

	if (setjmp(levelJmp) == 0) {
		dieRecoveryPoint = &levelJmp;
		initEngine(true);

		// initLevel only asks the audio task to start the music, which fails
		// out of sight; check the file is there.
		if (lhandle.sndfile[0] && !fileExists(configGetPath(lhandle.sndfile))) {
			ESP_LOGW(TAG, "music file %s not found", lhandle.sndfile);
		}

		stage = STAGE_PHYSICS;
		engineSelfTestPhysics();
		stage = STAGE_RENDER;
		engineSelfTestRender();

		dieRecoveryPoint = NULL;
		ok = true;
	} else {
		// A DIE() jumped here; dieWithError already disarmed the recovery point.
		snprintf(detail, detailLen, "FAILED in %s: %s", stageName[stage], dieRecoveryMessage);
		ok = false;

		allowLUAPaint = false;
		// A script that failed while being loaded never made it into lhandle,
		// so deInitLevel would not close it.
		if (scriptIsRunning && scriptIsRunning != lhandle.blLuaState &&
		    scriptIsRunning != lhandle.blOOLuaState && scriptIsRunning != lhandle.blConfigLuaState) {
			lua_close(scriptIsRunning);
		}
		scriptIsRunning = 0;
		SDL_FillRect(gScreen, NULL, 0xff000000);
	}
	return ok;
}

// ---- Entry point ------------------------------------------------------------

void runSelfTest() {
	soundStopMusic();
	flushJoystick();

	SELFTEST_ADDON* addons = (SELFTEST_ADDON*)calloc(SELFTEST_MAX_ADDONS, sizeof(SELFTEST_ADDON));
	SELFTEST_REPORT* reports = (SELFTEST_REPORT*)calloc(SELFTEST_MAX_REPORTS, sizeof(SELFTEST_REPORT));
	if (!addons || !reports) {
		ESP_LOGE(TAG, "out of memory");
		free(addons);
		free(reports);
		return;
	}

	Uint32 numAddons = readAddonList(addons, SELFTEST_MAX_ADDONS);
	ESP_LOGI(TAG, "==== SELF-TEST START: %u addon(s) ====", (unsigned)numAddons);

	testTask = xTaskGetCurrentTaskHandle();
	prevVprintf = esp_log_set_vprintf(selfTestVprintf);

	Uint32 numReports = 0;
	Uint32 totalLevels = 0, totalFailed = 0, totalWarned = 0;
	bool stopped = false;

	for (Uint32 a = 0; a < numAddons && !stopped; a++) {
		configSetAddOn(addons[a].dir);
		Uint32 numLevels = countLevels();
		ESP_LOGI(TAG, "[%s] \"%s\": %u level(s)", addons[a].dir, addons[a].name, (unsigned)numLevels);
		if (numLevels == 0) {
			ESP_LOGE(TAG, "[%s] FAILED: no level1.conf", addons[a].dir);
			totalFailed++;
			if (numReports < SELFTEST_MAX_REPORTS) {
				SELFTEST_REPORT* r = &reports[numReports++];
				snprintf(r->addon, sizeof(r->addon), "%s", addons[a].dir);
				r->level = 0;
				r->failed = true;
				snprintf(r->detail, sizeof(r->detail), "no level1.conf");
			}
		}

		for (Uint32 l = 1; l <= numLevels && !stopped; l++) {
			gamedata.player = &gamedata.players[0];
			gamedata.player->level = l;
			gamedata.player->score = 0;
			gamedata.player->lives = 2;
			isPauseMode = false;

			ESP_LOGI(TAG, "[%s] level %u/%u: loading", addons[a].dir, (unsigned)l, (unsigned)numLevels);
			logIssues = 0;
			firstIssue[0] = 0;

			char detail[256] = "";
			bool ok = runOneLevel(detail, sizeof(detail));
			int issues = logIssues;
			if (!ok) {
				// The DIE() itself logged one of the counted lines.
				issues = issues > 0 ? issues - 1 : 0;
			}

			char line1[160], line2[300];
			snprintf(line1, sizeof(line1), "%s: level %u of %u %s", addons[a].name,
			         (unsigned)l, (unsigned)numLevels, ok ? (issues ? "-- warnings" : "-- OK") : "-- FAILED");
			snprintf(line2, sizeof(line2), "%s", ok ? firstIssue : detail);
			drawStatus(line1, line2, !ok);

			deInitEngine();
			totalLevels++;

			if (!ok) {
				totalFailed++;
				ESP_LOGE(TAG, "[%s] level %u/%u: %s", addons[a].dir, (unsigned)l, (unsigned)numLevels, detail);
			} else if (issues) {
				totalWarned++;
				snprintf(detail, sizeof(detail), "%d warning/error log line(s), first: %s", issues, firstIssue);
				ESP_LOGW(TAG, "[%s] level %u/%u: OK, %s", addons[a].dir, (unsigned)l, (unsigned)numLevels, detail);
			} else {
				ESP_LOGI(TAG, "[%s] level %u/%u: OK", addons[a].dir, (unsigned)l, (unsigned)numLevels);
			}
			if ((!ok || issues) && numReports < SELFTEST_MAX_REPORTS) {
				SELFTEST_REPORT* r = &reports[numReports++];
				snprintf(r->addon, sizeof(r->addon), "%s", addons[a].dir);
				r->level = l;
				r->failed = !ok;
				r->issues = issues;
				snprintf(r->detail, sizeof(r->detail), "%s", detail);
			}
			// Leaks show up as this falling from level to level.
			ESP_LOGI(TAG, "[%s] level %u/%u: free PSRAM %u, largest block %u, free internal %u",
			         addons[a].dir, (unsigned)l, (unsigned)numLevels,
			         (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
			         (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
			         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

			if (stopRequested()) {
				stopped = true;
			}
		}
		configResetAddOn();
	}

	esp_log_set_vprintf(prevVprintf);

	ESP_LOGI(TAG, "==== SELF-TEST %s: %u level(s) in %u addon(s): %u OK, %u with warnings, %u failed ====",
	         stopped ? "STOPPED" : "SUMMARY", (unsigned)totalLevels, (unsigned)numAddons,
	         (unsigned)(totalLevels - totalFailed - totalWarned), (unsigned)totalWarned, (unsigned)totalFailed);
	for (Uint32 i = 0; i < numReports; i++) {
		SELFTEST_REPORT* r = &reports[i];
		if (r->failed) {
			ESP_LOGE(TAG, "  [%s] level %u: %s", r->addon, (unsigned)r->level, r->detail);
		} else {
			ESP_LOGW(TAG, "  [%s] level %u: %s", r->addon, (unsigned)r->level, r->detail);
		}
	}
	if (numReports == SELFTEST_MAX_REPORTS) {
		ESP_LOGW(TAG, "  (report list full; see the per-level lines above for the rest)");
	}
	ESP_LOGI(TAG, "==== SELF-TEST END ====");

	// Result screen
	char text[512];
	snprintf(text, sizeof(text),
	         "%s\n\nLevels tested: %u\nOK: %u\nWith warnings: %u\nFailed: %u\n\n"
	         "Details are in the debug log (tag \"selftest\").\n\nPress any key to return.",
	         stopped ? "Self-test stopped" : "Self-test finished",
	         (unsigned)totalLevels, (unsigned)(totalLevels - totalFailed - totalWarned),
	         (unsigned)totalWarned, (unsigned)totalFailed);
	SDL_FillRect(gScreen, NULL, 0xff000000);
	renderFontHandlerText(40, 40, text, totalFailed ? BS_Color_RED : BS_Color_WHITE,
	                      false, false, FONT_textfont_20);
	BS_Flip(gScreen);

	free(addons);
	free(reports);

	flushJoystick();
	while (!stopRequested()) {
		SDL_Delay(50);
	}
}
