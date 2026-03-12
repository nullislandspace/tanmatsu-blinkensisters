// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef EXTRACTMETABMF_H
#define EXTRACTMETABMF_H

void initExtractMetaBMF();
bool extractMetaBMF(char *fname, bool preStartup = false);
void extractMetaBMFprogress(Uint32 progress, bool preStartup);
void deInitExtractMetaBMF();
void registerItem(char *regStr, const char *filename);
void registerLevelconfig(const char *filename);
#endif // EXTRACTMETABMF_H



