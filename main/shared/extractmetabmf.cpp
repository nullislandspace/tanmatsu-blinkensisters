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
#include "progressui.h"

static const char* TAG = "extractmetabmf";

#define CHUNK_SIZE 4096

// The unpacking screen (progressui) follows the read position in the archive
// and redraws a few times a second at most. It used to redraw and flip the
// whole screen for every member, which for an archive of hundreds of small
// files was most of the time spent unpacking it.

static const char* baseName(const char* path) {
	const char* base = path;
	for (const char* p = path; *p; p++) {
		if (*p == '/' || *p == '\\') base = p + 1;
	}
	return base;
}

static void unpackProgress(FILE* ifh, long total, const char* bmfName, const char* member, bool force) {
	long pos = ftell(ifh);
	if (pos < 0) pos = 0;
	char line1[160];
	snprintf(line1, sizeof(line1), "Unpacking %s: %u%%", bmfName,
	         total > 0 ? (unsigned)((uint64_t)pos * 100 / (uint64_t)total) : 0u);
	progressUIDraw((uint64_t)pos, (uint64_t)(total > 0 ? total : 1), line1, member, force);
}

void initExtractMetaBMF() {
}

bool extractMetaBMF(char* fname, bool preStartup) {
	(void)preStartup;
	char infname[MAX_FNAME_LENGTH];
	const char* bmfName = baseName(fname);

	FILE* ifh = fastopen(fname, "rb");
	if(!ifh) {
		printf("Can't open file %s for input\n", fname);
		return false;
	}
	long total = -1;
	if (fseek(ifh, 0, SEEK_END) == 0) {
		total = ftell(ifh);
	}
	if (total < 0 || fseek(ifh, 0, SEEK_SET) != 0) {
		fastclose(ifh);
		DIE(ERROR_BMFEOF, fname);
	}
	ESP_LOGI(TAG, "Unpacking %s (%ld bytes)", fname, total);
	progressUIBegin(PROGRESSUI_UNPACK);
	unpackProgress(ifh, total, bmfName, "", true);

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
				if(tmpnum >= sizeof(infname) || !fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				MKDIR(configGetPath(infname));

				infname[0] = 0;
				break;
			case BMFTYPE_FILENAME:
				tmpnum = bmfReadInt(ifh);
				if(tmpnum >= sizeof(infname) || !fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				break;
			case BMFTYPE_REGISTER_ADDON:
				tmpnum = bmfReadInt(ifh);
				if(tmpnum >= sizeof(infname) || !fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"addons.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_REGISTER_MUSIC:
				tmpnum = bmfReadInt(ifh);
				if(tmpnum >= sizeof(infname) || !fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"playersounds.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_REGISTER_POSCAP:
				tmpnum = bmfReadInt(ifh);
				if(tmpnum >= sizeof(infname) || !fread(infname, tmpnum, 1, ifh)) {
					DIE(ERROR_BMFEOF, fname);
				}
				infname[tmpnum] = 0;
				registerItem(infname,"poscaps.dat");
				infname[0] = 0;
				break;
			case BMFTYPE_METAFILE_COMPRESSED:
			case BMFTYPE_FRAME_COMPRESSED:
			case BMFTYPE_SND_COMPRESSED:
				// exit() hangs on this device; say what is wrong instead.
				DIE(ERROR_BMFPARSE, "compressed BMF records are not supported");
				break;
			case BMFTYPE_METAFILE_UNCOMPRESSED:
				if(strlen(infname) == 0) {
					printf("Got file without filename!!\n");
					DIE(ERROR_FILE_WRITE, "No filename!");
				}
				{
					ESP_LOGD(TAG, "%s -> %s", bmfName, infname);
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
						if(fwrite(chunk, to_read, 1, ofh) != 1) {
							DIE(ERROR_FILE_WRITE, configGetPath(infname));
						}
						remaining -= to_read;
					}
					fastclose(ofh);
					unpackProgress(ifh, total, bmfName, infname, false);
				}

				infname[0] = 0; // Delete fname so we don't overwrite
				break;

			case BMFTYPE_END_OF_FILE:
				running = false;
				break;

			default:
				DIE(ERROR_BMFPARSE, fname);
		}
	}

	unpackProgress(ifh, total, bmfName, "", true);
	fastclose(ifh);
	progressUIEnd();

	return true;

}

void extractMetaBMFprogress(Uint32 progress, bool preStartup) {
	(void)progress;
	(void)preStartup;
}


void deInitExtractMetaBMF() {
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
