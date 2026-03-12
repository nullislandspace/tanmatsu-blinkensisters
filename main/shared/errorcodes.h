// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifdef PARSE_ERRORCODES
// The following lines are used via macros from errorhandler.*
// DO NOT INCLUDE THEM ELSEWHERE, INCLUDE errorhandler.h
XX(ERROR_FILE_READ,	 100, "Can't open file for reading")
XX(ERROR_FILE_WRITE,	101, "Can't open file for writing")
XX(ERROR_IMAGE_READ,	102, "Can't read or understand image file")
XX(ERROR_SOUND_READ,	103, "Can't read or understand sound file")
XX(ERROR_BOUNDARY,	  104, "Index out of boundary")
XX(ERROR_INDEX_INVALID, 105, "Invalid index")
XX(ERROR_POSITION,	  106, "Illegal Position")
XX(ERROR_FONTLOAD,	  117, "Could not load menu font")
XX(ERROR_LZO,	  118, "Error in LZO module")
XX(ERROR_BMFMAGIC,	  119, "BMF Magic mismatch")
XX(ERROR_BMFVERSION,	  120, "BMF Version mismatch")
XX(ERROR_BMFEOF,	  121, "Premature end of BMF file")
XX(ERROR_BMFWIDTH,	  122, "Unsupported frame width")
XX(ERROR_BMFHEIGHT,	  123, "Unsupported frame height")
XX(ERROR_BMFPARSE,	  124, "Failed to parse Meta BMF")
XX(ERROR_PLAYERSPRITE,125, "Player-sprite error")

XX(ERROR_HTTPGET,	  200, "Failed to get file from HTTP")
XX(ERROR_HTTPTOC,	  201, "TOC parse error")
XX(ERROR_HOME,        202, "Can't find users home directory")
XX(ERROR_MALLOC,      203, "Out of memory: malloc failed")
XX(ERROR_FILETEMPLATE,204, "Illegal filename template")

XX(ERROR_LUAINIT,	   307, "Could not initialize LUA")
XX(ERROR_LUALOAD,	   308, "Could not load LUA file")
XX(ERROR_LUAPARSE,	  309, "Could not parse LUA file")
XX(ERROR_LUACALL,	   310, "Error running LUA function")
XX(ERROR_LUANOINIT,	 311, "LUA not initialized")
XX(ERROR_LUAPARAMFORMAT,312, "Unknown LUA parameter format")
XX(ERROR_LUAARGCOUNT,   313, "Wrong number of arguments in LUA call")
XX(ERROR_LUARESULTTYPE, 314, "Wrong result type from LUA")
XX(ERROR_LUAL2BTYPE,	315, "Wrong type from LUA call to BlinkenSisters")
XX(ERROR_LUAL2BRANGE,	316, "LUA Argument out of range")
XX(ERROR_LUAL2BTEXTVALUE,	317, "Illegal text value")
XX(ERROR_LUAL2BGLOBALTYPE,	318, "Global has the incorrect type")
XX(ERROR_LUAL2BDIE,	 399, "DIE called from LUA script")


#endif // PARSE_ERRORCODES
