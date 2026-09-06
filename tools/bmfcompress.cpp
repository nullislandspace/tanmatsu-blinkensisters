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
#include <sys/types.h>
#include <sys/stat.h>

#include "bmfconvert.h"

#define IN_LEN 10000000

static unsigned char in  [ IN_LEN ];


int main(int argc, char *argv[])
{
	unsigned long in_len;
	bool isMeta = false;

	if(argc == 4 && strcmp(argv[1], "META") == 0) {
		printf("META!!!!\n");
		isMeta = true;
	} else if (argc != 6 && argc != 5){
		printf("Usage:\n"
               "bmfcompress fps framecount outputfile.bmf inputdir [soundfile]\n"
			   "   OR\n"
			   "bmfcompress META configfile outputfile.bmf\n");
		exit(3);
	}

	int framecount = atoi(argv[2]);

	unsigned long fulllength = 0;

    if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
        return 0;

	struct stat filestats;

	FILE* ofh = fopen(argv[3], "wb");
	if(!ofh) {
		printf("Can't write file %s\n", argv[3]);
		exit(1);
	}
	bmfWriteInt(ofh, BMFTYPE_MAGIC);
	bmfWriteInt(ofh, BMFTYPE_VERSION);
	bmfWriteInt(ofh, BMF_VERSION);
	fulllength += 3 * 4;

	if(!isMeta) {
		// --- normal Video BMF File ---
		printf("Compiling normal file...\n");
		bmfWriteInt(ofh, BMFTYPE_FPS);
		bmfWriteInt(ofh, atoi(argv[1]));
		bmfWriteInt(ofh, BMFTYPE_WIDTH);
		bmfWriteInt(ofh, SCR_WIDTH);
		bmfWriteInt(ofh, BMFTYPE_HEIGHT);
		bmfWriteInt(ofh, SCR_HEIGHT);
		bmfWriteInt(ofh, BMFTYPE_FRAMECOUNT);
		bmfWriteInt(ofh, framecount);
		fulllength += 4 * 8;

		// ++++++++++++++++++++++ SND ++++++++++++++++++++
		if(argc == 6) {
			bmfWriteInt(ofh, BMFTYPE_FILENAME);
			bmfWriteInt(ofh, strlen(argv[5]));
			fwrite(argv[5], strlen(argv[5]), 1, ofh);
			fulllength += 2 * 4 + strlen(argv[5]);

			char tmp1[500];
			sprintf(tmp1, "%s/%s", argv[4], argv[5]);

			if(stat(tmp1, &filestats)) {
				printf("Could not stat file %s\n", tmp1);
				exit(2);
			} else {
				in_len = filestats.st_size;
			}

			FILE* fh = fopen(tmp1, "rb");
			if(!fh) {
				printf("Could not open %s\n", tmp1);
				exit(1);
			}

			if(fread(in, in_len, 1, fh) != 1 ) {
				printf("Could not load %s\n", tmp1);
				exit(1);
			}
			fclose(fh);

			fulllength += in_len;
			fulllength += 8;
			bmfWriteInt(ofh, BMFTYPE_SND_UNCOMPRESSED);
			bmfWriteInt(ofh, in_len);
			fwrite(in, in_len, 1, ofh);
		}

		// +++++++++++++++++ FRAMES +++++++++++++++++++
		for(int i = 1; i <= framecount; i++) {
			char tmp1[500];
			char tmp2[500];
			tmp1[0] = 0;
			tmp2[0] = 0;
			sprintf(tmp1, "%d", i);
			while(strlen(tmp1) < 4) {
				sprintf(tmp2, "0%s", tmp1);
				sprintf(tmp1, "%s", tmp2);
			}
			strcat(tmp1, ".jpg");

			// Write filename
			bmfWriteInt(ofh, BMFTYPE_FILENAME);
			bmfWriteInt(ofh, strlen(tmp1));
			fwrite(tmp1, strlen(tmp1), 1, ofh);
			fulllength += 2 * 4 + strlen(tmp1);


			sprintf(tmp2, "%s/%s", argv[4], tmp1);
			sprintf(tmp1, "%s", tmp2);

			if(stat(tmp1, &filestats)) {
				printf("Could not stat file %s\n", tmp1);
				exit(2);
			} else {
				in_len = filestats.st_size;
			}

			FILE* fh = fopen(tmp1, "rb");
			if(!fh) {
				printf("Could not open %s\n", tmp1);
				exit(1);
			}

			if(fread(in, in_len, 1, fh) != 1 ) {
				printf("Could not load %s\n", tmp1);
				exit(1);
			}
			fclose(fh);

			fulllength += in_len;
			fulllength += 8;
			bmfWriteInt(ofh, BMFTYPE_FRAME_UNCOMPRESSED);
			bmfWriteInt(ofh, in_len);
			fwrite(in, in_len, 1, ofh);
		}

	} else {
		// META File that holds a BMF AddOn
		printf("Compiling AddOn (META) File...\n");
	    FILE* cfh = fopen(argv[2], "r");
		if(!cfh) {
			printf("Can't read config file!\n");
			exit(1);
		}
		char line[1001];
		char *tmp;
		char *cmd;
		char *filesrc;
		char *filedest;
		while(!feof(cfh)) {
			if(!fgets(line, 1000, cfh)) {
				break;
			}

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

			// Ok, let's try to parse command
			tmp = strchr(line, '=');
			if(!tmp) {
				printf("Unknown line '%s': Compress failed\n", line);
				return 3;
			}
			*tmp = '\0';
			cmd = line;
			filesrc = tmp;
			filesrc++;

			// ok, parse the command
			if(strcmp(cmd, "DIR") == 0) {
				bmfWriteInt(ofh, BMFTYPE_DIR);
				bmfWriteInt(ofh, strlen(filesrc));
				fwrite(filesrc, strlen(filesrc), 1, ofh);
				fulllength += 2 * 4 + strlen(filesrc);

			} else if(strcmp(cmd, "REGISTERADDON") == 0) {
					bmfWriteInt(ofh, BMFTYPE_REGISTER_ADDON);
					bmfWriteInt(ofh, strlen(filesrc));
					fwrite(filesrc, strlen(filesrc), 1, ofh);
					fulllength += 2 * 4 + strlen(filesrc);
			} else if(strcmp(cmd, "REGISTERMUSIC") == 0) {
					bmfWriteInt(ofh, BMFTYPE_REGISTER_MUSIC);
					bmfWriteInt(ofh, strlen(filesrc));
					fwrite(filesrc, strlen(filesrc), 1, ofh);
					fulllength += 2 * 4 + strlen(filesrc);
			} else if(strcmp(cmd, "REGISTERPOSCAP") == 0) {
				bmfWriteInt(ofh, BMFTYPE_REGISTER_POSCAP);
				bmfWriteInt(ofh, strlen(filesrc));
				fwrite(filesrc, strlen(filesrc), 1, ofh);
				fulllength += 2 * 4 + strlen(filesrc);
			} else if(strcmp(cmd, "FILE") == 0) {
				tmp = strchr(filesrc, '|');
				if(!tmp) {
					printf("File command has no pipe delimeter for srcfile/destfile disambiguation\n");
					return 4;
				}
				filedest = tmp;
				*tmp = '\0';
				filedest++;

				// Check file exists before writing anything
				if(stat(filesrc, &filestats)) {
					printf("WARNING: Skipping missing file %s\n", filesrc);
					continue;
				}
				in_len = filestats.st_size;

				FILE* fh = fopen(filesrc, "rb");
				if(!fh) {
					printf("WARNING: Skipping unreadable file %s\n", filesrc);
					continue;
				}

				if(fread(in, in_len, 1, fh) != 1 ) {
					printf("WARNING: Skipping unloadable file %s\n", filesrc);
					fclose(fh);
					continue;
				}
				fclose(fh);

				bmfWriteInt(ofh, BMFTYPE_FILENAME);
				bmfWriteInt(ofh, strlen(filedest));
				fwrite(filedest, strlen(filedest), 1, ofh);
				fulllength += 2 * 4 + strlen(filedest);

				fulllength += in_len;
				fulllength += 8;
				bmfWriteInt(ofh, BMFTYPE_METAFILE_UNCOMPRESSED);
				bmfWriteInt(ofh, in_len);
				fwrite(in, in_len, 1, ofh);

			} else {
				printf("Unknown command '%s'\n", cmd);
				return 5;
			}
		}

	}

	bmfWriteInt(ofh, BMFTYPE_END_OF_FILE);
	fulllength += 4;
	fclose(ofh);

	printf("\n\nFile should be %lu bytes long\n", fulllength);
	printf("  DONE  \n");

	return 0;
}
