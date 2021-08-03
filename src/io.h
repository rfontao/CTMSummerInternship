#ifndef _SEARCH_
#define _SEARCH_

#include "tables.h"

MtiaCylinder *readSingleCylinderFromFile(const char *file, int index);
MultiHashTable *readDatabase(const char *dir);
int readCylindersFromFile(const char *file, MtiaCylinder *out[]);
int readMatchScoreTemplates(const char *in1, const char *in2);
int loadMinutiaeListFromFile(const char *input_file_path, uint16_t **min_);
int readCylindersArrayFromFile(const char *file, Cylinders *out);

#endif