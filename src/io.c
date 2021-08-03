#include "io.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "tables.h"

// Reads all the files in a directory
// All files should be csv's
MultiHashTable* readDatabase(const char* dir) {
  DIR* db_dir;
  MultiHashTable* db;
  struct dirent* in_file;

  if ((db_dir = opendir(dir)) == NULL) {
    fprintf(stderr, "Could not open db dir - %s\n", dir);
    return NULL;
  }

  // Counting total database entries
  size_t entries_count = 0;
  while ((in_file = readdir(db_dir))) {
    // TODO Doesnt check for dirs to ignore
    if (strcmp(in_file->d_name, ".") && strcmp(in_file->d_name, "..")) {
      ++entries_count;
    }
  }
  printf("Found %lu files to load\n", entries_count);

  rewinddir(db_dir);

  // TODO hardcoded m
  db = initHashTable(MCC_SIZE, 80, entries_count);
  if (db == NULL) {
    closedir(db_dir);
    return NULL;
  }

  size_t count = 0;
  while ((in_file = readdir(db_dir))) {
    // TODO Doesnt check for dirs to ignore
    if (strcmp(in_file->d_name, ".") && strcmp(in_file->d_name, "..")) {
      // Concatenate dir and file name
      char* relative_dir = (char*)malloc(
          (strlen(dir) + strlen(in_file->d_name) + 2) * sizeof(char));
      // TODO Checks
      strcpy(relative_dir, dir);
      strcat(relative_dir, "/");
      strcat(relative_dir, in_file->d_name);

      MtiaCylinder* cyls[MAX_MINUTIAE_PER_FP];
      int read = readCylindersFromFile(relative_dir, cyls);
      if (read == -1) {
        free(relative_dir);
        closedir(db_dir);
        destroyHashTable(db);
        return NULL;
      }

      db->template_map[count].template = relative_dir;
      db->template_map[count].min_amount = read;
      for (size_t i = 0; i < read; ++i) {
        cyls[i]->template_index = count;
        cyls[i]->minutiae_index = i;
      }

      // free(relative_dir);
      for (int i = 0; i < read; ++i) {
        if (addCylinder(db, cyls[i]) < 0) {
          closedir(db_dir);
          destroyHashTable(db);
          return NULL;
        }
      }
      // printf("Loaded file %lu of %lu - %s\n", count + 1, entries_count,
      //        in_file->d_name);
      count++;
    }
  }

  closedir(db_dir);
  printf("Finished reading db\n");
  return db;
}

// Returns amount of read cylinders or -1 in case of error
int readCylindersFromFile(const char* file, MtiaCylinder* out[]) {
  FILE* in = fopen(file, "r");
  if (in == NULL) {
    fprintf(stderr, "Could not open file %s\n", file);
    return -1;
  }
  int index = 0;
  int x, y, a, t, q;
  while (fscanf(in, "%d,%d,%d,%d,%d,", &x, &y, &a, &t, &q) != EOF) {
    MtiaCylinder* cyl = (MtiaCylinder*)malloc(sizeof(MtiaCylinder));
    if (cyl == NULL) {
      fclose(in);
      return -1;
    }
    cyl->minutia.x = x;
    cyl->minutia.y = y;
    cyl->minutia.angle = a;
    cyl->minutia.type = t;
    cyl->minutia.quality = q;

    for (size_t i = 0; i < MCC_VALUE_SIZE; ++i) {
      fscanf(in, "%d,", &(cyl->values[i]));
    }

    set_popcount(cyl);

    out[index++] = cyl;
  }

  fclose(in);
  return index;
}

int readCylindersArrayFromFile(const char* file, Cylinders* out) {
  FILE* in = fopen(file, "r");
  if (in == NULL) {
    fprintf(stderr, "Could not open file %s\n", file);
    return -1;
  }
  int index = 0;
  int x, y, a, t, q;
  while (fscanf(in, "%d,%d,%d,%d,%d,", &x, &y, &a, &t, &q) != EOF) {
    MtiaCylinder cyl;
    cyl.minutia.x = x;
    cyl.minutia.y = y;
    cyl.minutia.angle = a;
    cyl.minutia.type = t;
    cyl.minutia.quality = q;

    for (size_t i = 0; i < MCC_VALUE_SIZE; ++i) {
      fscanf(in, "%d,", &(cyl.values[i]));
    }

    set_popcount(&cyl);

    out->MtiaCylinders[index++] = cyl;
  }
  out->count = index;

  fclose(in);
  return index;
}

MtiaCylinder* readSingleCylinderFromFile(const char* file, int index) {
  FILE* in = fopen(file, "r");
  if (in == NULL) {
    fprintf(stderr, "Could not open file %s\n", file);
    return NULL;
  }

  int counter = 0;
  int x, y, a, t, q;
  while (fscanf(in, "%d,%d,%d,%d,%d,", &x, &y, &a, &t, &q) != EOF) {
    MtiaCylinder* cyl = (MtiaCylinder*)malloc(sizeof(MtiaCylinder));
    if (cyl == NULL) {
      fclose(in);
      return NULL;
    }
    cyl->minutia.x = x;
    cyl->minutia.y = y;
    cyl->minutia.angle = a;
    cyl->minutia.type = t;
    cyl->minutia.quality = q;

    for (size_t i = 0; i < MCC_VALUE_SIZE; ++i) {
      fscanf(in, "%d,", &(cyl->values[i]));
    }

    set_popcount(cyl);

    if (index == counter) {
      fclose(in);
      return cyl;
    } else {
      free(cyl);
      counter++;
    }
  }

  fclose(in);
  return NULL;
}

int loadMinutiaeListFromFile(const char* input_file_path, uint16_t** min_) {
  int i, x, y, a, t, q;
  uint16_t* min;
  FILE* in;

  // Cheking the output pointer is not NULL ?
  if (!min_) {
    return -1;
  }

  // Opening the input file
  in = fopen(input_file_path, "r");
  if (in == NULL) {
    return -1;
  }

  min = (uint16_t*)malloc(
      sizeof(uint16_t) *
      (MAX_MINUTIAE_PER_TEMPLATE * FEATURE_AMOUNT + 1));  // +1 for i?
  if (!min) {
    fclose(in);
    return -1;
  }
  *min_ = min;

  min++;  // Skips first element to store amount after reading
  i = 0;
  while (fscanf(in, "%d %d %d %d %d", &x, &y, &a, &t, &q) == FEATURE_AMOUNT &&
         i < MAX_MINUTIAE_PER_TEMPLATE) {
    *min = x;
    min++;
    *min = y;
    min++;
    *min = a;
    min++;
    *min = t;
    min++;
    *min = q;
    min++;
    i++;
  }
  **min_ = i;  // First value is the amount of read minutiae
  fclose(in);
  return 0;
}

int readMatchScoreTemplates(const char* in1, const char* in2) {
  // minutiae
  uint16_t *min1, *min2;

  if (loadMinutiaeListFromFile(in1, &min1) < 0) {
    fprintf(stderr, "Could not extract minutae list from file %s", in1);
    return -1;
  }

  if (loadMinutiaeListFromFile(in2, &min2) < 0) {
    fprintf(stderr, "Could not extract minutae list from file %s", in2);
    free(min1);
    return -1;
  }

  int score = match_from_array(min1, min2);

  free(min1);
  free(min2);

  return score;
}
