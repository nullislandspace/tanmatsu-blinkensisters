// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef CMDOPTS_H
#define CMDOPTS_H

bool initCmdOpt(int ARGC, char **ARGV);
int getCmdOptBool(char* name);
char* getCmdOptString(char* name);
void helpCmdOpt(char* name);

struct CMD_OPTS {
	char shortname[255];
	char fullname[255];
	char description[255];
	bool isBool;
	int  defaultBool; /* HOW OFTEN was a boolean value found - allows "-v -v", ... */
	char * defaultString;
};

#define CMD_OPTS_END {"END", "END", "END", false, 0, (char*)"END"}

#endif // CMDOPTS_H
