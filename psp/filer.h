#ifndef FILER_H
#define FILER_H

extern char LastPath[], FilerMsg[];

int getExtId(const char *szFilePath);
int searchFile(const char *path, const char *name);
int getFilePath(char*,char*,int*);

// —LŒø‚ÈŠg’£Žq
enum {
  EXT_NULL,
  EXT_UNKNOWN,
  EXT_ALL,
  EXT_GB,
  EXT_GBC,
  EXT_ZIP,
  EXT_PCE,
  EXT_TOC,
};



#endif
