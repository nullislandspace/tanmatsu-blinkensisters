// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// See License.txt for licensing information
//
// bmfextract -- read a .bmf archive: list what is in it, or unpack it.
//
// The counterpart to bmfcompress. Between the two, a shipped .bmf can be
// opened up, a file fixed or added, and the archive rebuilt -- without
// needing the original asset tree, since everything is already inside.
//
// The format is described in libs/bmf/README_BMF in the upstream project and
// implemented by main/bmf/bmfconvert.*, which this shares. In short: a magic
// and version header, then a sequence of 4-byte tagged records, then
// BMFTYPE_END_OF_FILE. FILENAME and DIR records carry a length-prefixed
// string; FRAME, SND and METAFILE records carry length-prefixed payload that
// belongs to the FILENAME most recently seen.
//
// The _COMPRESSED record types are recognised but not supported: nothing in
// this project's data uses them (the upstream compressor only ever emits the
// uncompressed forms), and guessing at a codec that is never exercised would
// be worse than saying so.

#include "bmfconvert.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

#define MAX_NAME 1024

static void usage(void) {
    fprintf(stderr,
        "Usage:\n"
        "  bmfextract list <file.bmf>\n"
        "  bmfextract extract <file.bmf> <outdir>\n"
        "  bmfextract cat <file.bmf> <name>      (write one member to stdout)\n"
        "\n"
        "'list' prints one line per member: type, size and name.\n"
        "'extract' unpacks members into outdir, creating subdirectories as the\n"
        "archive asks for them.\n");
}

// Create every directory along `path`, ignoring ones that already exist.
static bool mkdir_recursive(const char* path) {
    char tmp[MAX_NAME];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char* p = tmp + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (MKDIR(tmp) != 0 && errno != EEXIST) {
            fprintf(stderr, "Cannot create directory %s: %s\n", tmp, strerror(errno));
            return false;
        }
        *p = '/';
    }
    if (MKDIR(tmp) != 0 && errno != EEXIST) {
        fprintf(stderr, "Cannot create directory %s: %s\n", tmp, strerror(errno));
        return false;
    }
    return true;
}

// Create the directories leading up to a file path (not the file itself).
static bool mkdir_for_file(const char* path) {
    char tmp[MAX_NAME];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char* slash = strrchr(tmp, '/');
    if (!slash) {
        return true;
    }
    *slash = '\0';
    return tmp[0] == '\0' ? true : mkdir_recursive(tmp);
}

// Read a length-prefixed string record into `out`.
static bool read_string(FILE* fh, char* out, size_t outsize) {
    unsigned int len = bmfReadInt(fh);
    if (len >= outsize) {
        fprintf(stderr, "Name of %u bytes is too long\n", len);
        return false;
    }
    if (len > 0 && fread(out, len, 1, fh) != 1) {
        fprintf(stderr, "Truncated while reading a name\n");
        return false;
    }
    out[len] = '\0';
    return true;
}

static const char* type_name(unsigned int type) {
    switch (type) {
        case BMFTYPE_FPS:                    return "fps";
        case BMFTYPE_FRAMECOUNT:             return "framecount";
        case BMFTYPE_WIDTH:                  return "width";
        case BMFTYPE_HEIGHT:                 return "height";
        case BMFTYPE_FILENAME:               return "filename";
        case BMFTYPE_FRAME_COMPRESSED:       return "frame(z)";
        case BMFTYPE_FRAME_UNCOMPRESSED:     return "frame";
        case BMFTYPE_SND_COMPRESSED:         return "snd(z)";
        case BMFTYPE_SND_UNCOMPRESSED:       return "snd";
        case BMFTYPE_METAFILE_COMPRESSED:    return "file(z)";
        case BMFTYPE_METAFILE_UNCOMPRESSED:  return "file";
        case BMFTYPE_DIR:                    return "dir";
        case BMFTYPE_REGISTER_ADDON:         return "reg-addon";
        case BMFTYPE_REGISTER_MUSIC:         return "reg-music";
        case BMFTYPE_REGISTER_POSCAP:        return "reg-poscap";
        case BMFTYPE_END_OF_FILE:            return "eof";
        default:                             return "?";
    }
}

typedef enum { MODE_LIST, MODE_EXTRACT, MODE_CAT } mode_t_;

static int run(mode_t_ mode, const char* bmfpath, const char* arg) {
    FILE* fh = fopen(bmfpath, "rb");
    if (!fh) {
        fprintf(stderr, "Cannot open %s: %s\n", bmfpath, strerror(errno));
        return 1;
    }

    if (bmfReadInt(fh) != BMFTYPE_MAGIC) {
        fprintf(stderr, "%s is not a BMF file (bad magic)\n", bmfpath);
        fclose(fh);
        return 2;
    }
    if (bmfReadInt(fh) != BMFTYPE_VERSION) {
        fprintf(stderr, "%s: expected a version record\n", bmfpath);
        fclose(fh);
        return 2;
    }
    unsigned int version = bmfReadInt(fh);
    if (version != BMF_VERSION) {
        fprintf(stderr, "%s is BMF version %u, this tool speaks %u\n",
                bmfpath, version, BMF_VERSION);
        fclose(fh);
        return 2;
    }
    if (mode == MODE_LIST) {
        printf("%s: BMF version %u\n", bmfpath, version);
    }

    char curname[MAX_NAME] = "";
    unsigned int members = 0;
    unsigned long long total = 0;
    bool found = false;

    for (;;) {
        unsigned int type = bmfReadInt(fh);
        if (feof(fh)) {
            fprintf(stderr, "Unexpected end of file (no BMFTYPE_END_OF_FILE)\n");
            fclose(fh);
            return 3;
        }
        if (type == BMFTYPE_END_OF_FILE) {
            break;
        }

        switch (type) {
            case BMFTYPE_FPS:
            case BMFTYPE_FRAMECOUNT:
            case BMFTYPE_WIDTH:
            case BMFTYPE_HEIGHT: {
                unsigned int value = bmfReadInt(fh);
                if (mode == MODE_LIST) {
                    printf("  %-10s %u\n", type_name(type), value);
                }
                break;
            }

            case BMFTYPE_FILENAME:
                if (!read_string(fh, curname, sizeof(curname))) {
                    fclose(fh);
                    return 3;
                }
                break;

            case BMFTYPE_DIR:
            case BMFTYPE_REGISTER_ADDON:
            case BMFTYPE_REGISTER_MUSIC:
            case BMFTYPE_REGISTER_POSCAP: {
                char name[MAX_NAME];
                if (!read_string(fh, name, sizeof(name))) {
                    fclose(fh);
                    return 3;
                }
                if (mode == MODE_LIST) {
                    printf("  %-10s %s\n", type_name(type), name);
                } else if (mode == MODE_EXTRACT && type == BMFTYPE_DIR) {
                    char path[MAX_NAME];
                    snprintf(path, sizeof(path), "%s/%s", arg, name);
                    if (!mkdir_recursive(path)) {
                        fclose(fh);
                        return 4;
                    }
                }
                break;
            }

            case BMFTYPE_FRAME_COMPRESSED:
            case BMFTYPE_SND_COMPRESSED:
            case BMFTYPE_METAFILE_COMPRESSED:
                fprintf(stderr, "%s: '%s' uses a compressed record, which this "
                                "tool does not implement\n", bmfpath, curname);
                fclose(fh);
                return 5;

            case BMFTYPE_FRAME_UNCOMPRESSED:
            case BMFTYPE_SND_UNCOMPRESSED:
            case BMFTYPE_METAFILE_UNCOMPRESSED: {
                unsigned int len = bmfReadInt(fh);
                members++;
                total += len;

                if (mode == MODE_LIST) {
                    printf("  %-10s %9u  %s\n", type_name(type), len,
                           curname[0] ? curname : "(unnamed)");
                    if (fseek(fh, (long)len, SEEK_CUR) != 0) {
                        fprintf(stderr, "Truncated payload for %s\n", curname);
                        fclose(fh);
                        return 3;
                    }
                    break;
                }

                bool want = (mode == MODE_EXTRACT) ||
                            (mode == MODE_CAT && strcmp(curname, arg) == 0);
                if (!want) {
                    if (fseek(fh, (long)len, SEEK_CUR) != 0) {
                        fprintf(stderr, "Truncated payload for %s\n", curname);
                        fclose(fh);
                        return 3;
                    }
                    break;
                }

                unsigned char* buf = (unsigned char*)malloc(len ? len : 1);
                if (!buf) {
                    fprintf(stderr, "Out of memory for %u bytes\n", len);
                    fclose(fh);
                    return 4;
                }
                if (len > 0 && fread(buf, len, 1, fh) != 1) {
                    fprintf(stderr, "Truncated payload for %s\n", curname);
                    free(buf);
                    fclose(fh);
                    return 3;
                }

                if (mode == MODE_CAT) {
                    fwrite(buf, len, 1, stdout);
                    found = true;
                    free(buf);
                    fclose(fh);
                    return 0;
                }

                if (curname[0] == '\0') {
                    fprintf(stderr, "WARNING: skipping a %u byte member with no "
                                    "filename record\n", len);
                    free(buf);
                    break;
                }

                char path[MAX_NAME];
                snprintf(path, sizeof(path), "%s/%s", arg, curname);
                if (!mkdir_for_file(path)) {
                    free(buf);
                    fclose(fh);
                    return 4;
                }
                FILE* out = fopen(path, "wb");
                if (!out) {
                    fprintf(stderr, "Cannot write %s: %s\n", path, strerror(errno));
                    free(buf);
                    fclose(fh);
                    return 4;
                }
                if (len > 0) {
                    fwrite(buf, len, 1, out);
                }
                fclose(out);
                free(buf);
                printf("  %9u  %s\n", len, curname);
                break;
            }

            default:
                fprintf(stderr, "Unknown record type 0x%08x after '%s'; the file "
                                "may be corrupt\n", type, curname);
                fclose(fh);
                return 3;
        }
    }

    fclose(fh);

    if (mode == MODE_CAT && !found) {
        fprintf(stderr, "'%s' is not in %s\n", arg, bmfpath);
        return 6;
    }
    if (mode != MODE_CAT) {
        printf("%u members, %llu bytes of payload\n", members, total);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && strcmp(argv[1], "list") == 0) {
        return run(MODE_LIST, argv[2], NULL);
    }
    if (argc == 4 && strcmp(argv[1], "extract") == 0) {
        if (!mkdir_recursive(argv[3])) {
            return 4;
        }
        return run(MODE_EXTRACT, argv[2], argv[3]);
    }
    if (argc == 4 && strcmp(argv[1], "cat") == 0) {
        return run(MODE_CAT, argv[2], argv[3]);
    }
    usage();
    return 1;
}
