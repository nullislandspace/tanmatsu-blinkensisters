#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include "globals.h"
#include <setjmp.h>

#define PARSE_ERRORCODES
#define XX(errenum, errcode, errtxt) errenum = errcode,
typedef enum _ERRORCODES {
#include "errorcodes.h"
    ERROR_LASTERROR
} ERRORCODES;
#undef XX
#undef PARSE_ERRORCODES

#define displaymessage_INFO    1
#define displaymessage_WARNING 2
#define displaymessage_ERROR   3

char* getErrorText(Uint32 errorcode);

#define DIE(err, txt) dieWithError(err, txt, __LINE__, __FILE__)
void dieWithError(Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename);
void displayErrorOnScreen(Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename);
void displaymessage(int messagetype, const char* message, Uint32 delay_ms);
extern bool displayGraphicalErrors;

// Recoverable DIE(), for the self-test. While dieRecoveryPoint is set, DIE()
// logs the error, copies it to dieRecoveryMessage, clears dieRecoveryPoint and
// longjmp()s there instead of ending the program. The caller owns cleaning up
// whatever the interrupted code had half built.
extern jmp_buf* dieRecoveryPoint;
extern char dieRecoveryMessage[512];

#endif // ERRORHANDLER_H
