#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "io.h"
#include "tables.h"

int main(int argc, char const* argv[]) {
  if (argc != 4) {
    printf(
        "Usage: %s <Database Directory> <Query File> <K(Candidate amount)>\n",
        argv[0]);
    return -1;
  }

  MultiHashTable* db = readDatabase(argv[1]);
  if (db == NULL) {
    fprintf(stderr, "Could not load database from %s\n", argv[1]);
    return -1;
  }

  MtiaCylinder* out[MAX_MINUTIAE_PER_FP];
  int read = readCylindersFromFile(argv[2], out);
  if (read == -1) {
    destroyHashTable(db);
    return -1;
  }

  int k = atoi(argv[3]);

  SRecord* res = search_template(db, out, read, k);
  printf("Best %d results:\n", k);
  for (size_t i = 0; i < k; i++) {
    printf("%s - %f\n", db->template_map[res[i].template_index].template,
           res[i].score);
  }

  free(res);
  destroyHashTable(db);

  return 0;
}
