// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See LICENSE for licensing information
//


#include "globals.h"
#include "cmdopts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern CMD_OPTS cmd_opt_data[];

/* Parse command line parameters and put the values in the structure */
bool initCmdOpt(int ARGC, char **ARGV) {
	bool paramOK = true;
	
	for(int i = 1; i < ARGC; i++) {
		CMD_OPTS *it = cmd_opt_data;
		bool found = false;
		while(strcmp(it->shortname, "END") != 0) {
			if(strncmp(ARGV[i], it->shortname,strlen(it->shortname)) == 0 || 
			   strncmp(ARGV[i], it->fullname,strlen(it->fullname)) == 0) {
				found = true;
				if(it->isBool) { /* Boolean value */
					it->defaultBool++;
				} else { /* String value */
					if (strchr(ARGV[i],'=')==NULL) { /* No equal sign after parameter */
						paramOK = false; /* String without "=" after option */
					} else { /* equal sign is there */
						it->defaultString = strchr(ARGV[i],'=')+sizeof(char) ; 
					}
				}
			}
			it++;
		}
		if(!found) {
			fprintf(stderr,"Unknown parameter: %s\n", ARGV[i]);
			paramOK = false;
		}
		
	}
	return paramOK;
}

/* get a boolean value. Return value is an *integer* - how often was a boolean value given */
/* This allows something like "program -v" verbose, "program -v -v" more verbose */
/* return -1 in case of an error */
int getCmdOptBool(char* name) {
	CMD_OPTS *it = cmd_opt_data;
	
	while(strcmp(it->shortname, "END") != 0) {
		if(strcmp(name, it->shortname) == 0 || strcmp(name, it->fullname) == 0) {
			if(it->isBool) {
				return (it->defaultBool); /* Return the result */
			} else {
				fprintf(stderr,"Parameter %s is not a BOOL!\n", name); /* Error */
				return(-1);
			}
		}
		it++ ;
	};
	/* Value was not found - error */
	fprintf(stderr,"Unknown parameter: %s\n", name);
	return(-1);
}

/* parse a string option. Return NULL in case of an error */
/* expects the string after a "=" sign after the option name 
   (both when using short and long options.
   For example:
   "--exampleoption=examplestring", "-e=examplestring" 

   Warning: This is not the same than the usual GNU getopt concept.
   There you have "--exampleoption=examplestring" but "-e examplestring". */
char* getCmdOptString(char* name) {
	CMD_OPTS *it = cmd_opt_data;
	while(strcmp(it->shortname, "END") != 0) {
		if(strcmp(name, it->shortname) == 0 || strcmp(name, it->fullname) == 0) {
			if(it->isBool == false) {
				return(it->defaultString);
			} else {
				fprintf(stderr,"Parameter %s is not a STRING!\n", name);
				return(NULL);
			}
		}
		it++;
	}
	/* Value was not found - error */	
	fprintf(stderr,"Unknown parameter: %s\n", name);
	return(NULL);
}

/* output help text, the argument is the name of the executable (determined from argv[0] */
void helpCmdOpt(char* name) {
	printf("Usage: %s [Options]\n",name);
	CMD_OPTS *it = cmd_opt_data;
	
	while(strcmp(it->shortname, "END") != 0) {
		printf("  %s, %-24s %s\n", it->shortname, it->fullname, it->description);
		it++;
	}
}

