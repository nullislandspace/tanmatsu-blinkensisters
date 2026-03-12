// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#else // NO WIN32
#include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>


#include "globals.h"
#include "extractmetabmf.h"
#include "debug.h"
#include "errorhandler.h"

#include "fastopen.h"
#include "bmfconvert.h"
#include "config.h"
#include "drawprimitives.h"
#include "bsscreen.h"
#include "fonthandler.h"
#include "osdef.h"
#include "esp_log.h"

static const char* TAG = "extractmetabmf";

#define CHUNK_SIZE 4096

SDL_Surface* decrunchingbg = 0;

// Show extraction status on screen (used during preStartup when normal progress is disabled)
static void showExtractionStatus(const char* bmf_path, const char* writing_file) {
    // Extract just the filename from the BMF path for readability
    const char* bmf_name = bmf_path;
    for (const char* p = bmf_path; *p; p++) {
        if (*p == '/' || *p == '\\') bmf_name = p + 1;
    }

    if (writing_file && writing_file[0]) {
        ESP_LOGI(TAG, "Extracting %s -> writing: %s", bmf_name, writing_file);
    } else {
        ESP_LOGI(TAG, "Extracting %s ...", bmf_name);
    }

    if (!gScreen) {
        ESP_LOGE(TAG, "gScreen is NULL, cannot render status");
        return;
    }

    SDL_FillRect(gScreen, NULL, 0xff000000);

    SDL_Color white = {255, 255, 255, 255};
    renderFontHandlerText(SCR_WIDTH / 2, SCR_HEIGHT / 2 - 40,
                          "Extracting game data...",
                          white, true, false, FONT_textfont_20);

    char line[512];
    snprintf(line, sizeof(line), "BMF: %s", bmf_name);
    renderFontHandlerText(SCR_WIDTH / 2, SCR_HEIGHT / 2,
                          line, white, true, false, FONT_textfont_20);

    if (writing_file && writing_file[0]) {
        snprintf(line, sizeof(line), "Writing: %s", writing_file);
        renderFontHandlerText(SCR_WIDTH / 2, SCR_HEIGHT / 2 + 40,
                              line, white, true, false, FONT_textfont_20);
    }

    int flip_ret = BS_Flip(gScreen);
    ESP_LOGI(TAG, "BS_Flip returned %d", flip_ret);
}

void initExtractMetaBMF() {
#ifndef DISABLE_BACKGROUND_ART
	decrunchingbg = BS_IMG_Load_DisplayFormat(configGetPath("decrunchingbg.png"), false);
#endif
}


bool extractMetaBMF(char* fname, bool preStartup) {
	char infname[MAX_FNAME_LENGTH];
	extractMetaBMFprogress(0, preStartup);
	showExtractionStatus(fname, "");

	FILE* ifh = fastopen(fname, "rb");
	if(!ifh) {
		printf("Can't open file %s for input\n", fname);
		return false;
	}
	Uint32 tmpnum;
	Uint32 bmftype;

	// Check magic number
	tmpnum = bmfReadInt(ifh);
	if(tmpnum != BMFTYPE_MAGIC) {
		DIE(ERROR_BMFMAGIC, fname);
	}

	bool running = true;
	FILE *ofh;
	static unsigned char chunk[CHUNK_SIZE];
	while(running) {
		bmftype = bmfReadInt(ifh);
		switch(bmftype) {
			case BMFTYPE_VERSION:
				tmpnum = bmfReadInt(ifh);
				if(tmpnum != BMF_VERSION) {
					if(tmpnum == 1) {
						printf("Warning: Older (compatible) Version1 BMF found - continuing\n");
					} else {
						DIE(ERROR_BMFVERSION, fname);
					}
				}
				break;
			case BMFTYPE_DIR:
				tmpnum = bmfReadInt(ifh);
				if(!fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				MKDIR(configGetPath(infname));

				infname[0] = 0;
				break;
			case BMFTYPE_FILENAME:
				tmpnum = bmfReadInt(ifh);
				if(!fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				break;
			case BMFTYPE_REGISTER_ADDON:
				tmpnum = bmfReadInt(ifh);
				if(!fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"addons.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_REGISTER_MUSIC:
				tmpnum = bmfReadInt(ifh);
				if(!fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"playersounds.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_REGISTER_POSCAP:
				tmpnum = bmfReadInt(ifh);
				if(!fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"poscaps.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_METAFILE_COMPRESSED:
			case BMFTYPE_FRAME_COMPRESSED:
			case BMFTYPE_SND_COMPRESSED:
				printf("Error: Compressed BMF data not supported (file: %s)\n", fname);
				fastclose(ifh);
				exit(1);
				break;
			case BMFTYPE_METAFILE_UNCOMPRESSED:
				if(strlen(infname) == 0) {
					printf("Got file without filename!!\n");
					DIE(ERROR_FILE_WRITE, "No filename!");
				}
				{
					showExtractionStatus(fname, infname);
					Uint32 file_len = bmfReadInt(ifh);

					// Write file using chunked reading
					ofh = fastopen(configGetPath(infname), "wb");
					if(!ofh) {
						DIE(ERROR_FILE_WRITE, configGetPath(infname));
					}
					Uint32 remaining = file_len;
					while(remaining > 0) {
						Uint32 to_read = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
						if(!fread(chunk, to_read, 1, ifh)) {
							DIE(ERROR_BMFEOF, fname);
						}
						fwrite(chunk, to_read, 1, ofh);
						remaining -= to_read;
					}
					fastclose(ofh);
				}

				infname[0] = 0; // Delete fname so we don't overwrite
				break;

			case BMFTYPE_END_OF_FILE:
				running = false;
				break;

			default:
				printf("Unknown BMFTYPE %d...\n", bmftype);
				exit(1);
		}
	}

	fastclose(ifh);

	return true;

}

void extractMetaBMFprogress(Uint32 progress, bool preStartup) {

     // Avoid compiler warning about unused argument  FIXME
     progress = 0;

	if(preStartup) {
		return;
	}

#ifdef DISABLE_BACKGROUND_ART
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
	if (SDL_MUSTLOCK(decrunchingbg))
		SDL_UnlockSurface(decrunchingbg);
	if (SDL_MUSTLOCK(gScreen))
		SDL_UnlockSurface(gScreen);

	SDL_BlitSurface( decrunchingbg,  NULL, gScreen, NULL );
#endif

	BS_Flip(gScreen); /* Update whole screen */
}


void deInitExtractMetaBMF() {
#ifndef DISABLE_BACKGROUND_ART
	SDL_FreeSurface(decrunchingbg);
	decrunchingbg = 0;
#endif
}

/* register item (Addon, Music, Poscap) in file */
/* check if regStr exists in file filename and - if not - add it */
void registerItem(char *regStr, const char *filename) {
	static char regLines[100][1000];
	memset(regLines, 0, sizeof(regLines));
	FILE* fh;
	char line[1000];
	char *tmp;
	Uint32 cnt = 0;
	bool found = false;
	fh = fastopen(configGetPath(filename), "r");
	if(fh) {
		while(fgets(line, sizeof(line), fh)) {
			// Remove Comments and newline characters
			while((tmp = strchr(line, '\n'))) {
				*tmp = 0;
			}
			while((tmp = strchr(line, '\r'))) {
				*tmp = 0;
			}
			while((tmp = strchr(line, '#'))) {
				*tmp = 0;
			}

			if(line[0] == '\0') {
				// Ignore remark or empty line
				continue;
			}
			sprintf(regLines[cnt], "%s", line);
			cnt++;
		}
		fastclose(fh);

		for(Uint32 i = 0; i < cnt; i++) {
			if(strcmp(regStr, regLines[i]) == 0) {
				found = true;
			}
		}
	}

	if(!found) {
		sprintf(regLines[cnt], "%s", regStr);
		cnt++;

		fh = fastopen(configGetPath(filename), "w");
		if(!fh) {
			DIE(ERROR_FILE_WRITE, configGetPath(filename));
		}
		for(Uint32 i = 0; i < cnt; i++) {
			fprintf(fh, "%s\r\n", regLines[i]);
		}
		fastclose(fh);
	}
	return;
}

/* Read Level-"config"-file line by line. Check for lines starting with
"REGISTERADDON=" or "REGISTERMUSIC=" and register these items */
void registerLevelconfig(const char *filename) {
	FILE* fh;
	char line[1000];
	fh = fastopen(filename, "r");
	if(fh) {
		while(fgets(line, sizeof(line), fh)) {
			line[strlen(line)-1]='\0' ; /* Remove last character (\n') */
			if(strncmp(line, "REGISTERADDON=",sizeof("REGISTERADDON=")-1) == 0) {
				registerItem(line+sizeof("REGISTERADDON=")-1,"addons.dat");
			}
			if(strncmp(line, "REGISTERMUSIC=",sizeof("REGISTERMUSIC=")) == 0) {
				registerItem(line+sizeof("REGISTERMUSIC=")-1,"playersounds.dat");
			}
		}
		fastclose(fh);
	}
}
