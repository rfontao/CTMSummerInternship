#include <stdio.h>
#include <time.h>

#include "io.h"
#include "tables.h"

int main(int argc, char const *argv[]) {
  if (argc != 5) {
    printf("Usage: %s <Database path> <queries> <out_candidates> <k>\n",
           argv[0]);
    return -1;
  }

  int k = atoi(argv[4]);

  MultiHashTable *db = readDatabase(argv[1]);
  if (db == NULL) {
    fprintf(stderr, "Could not load database from %s\n", argv[1]);
    return -1;
  }

  FILE *queries = fopen(argv[2], "r");
  if (queries == NULL) {
    printf("Failed to open queries file\n");
    return -1;
  }

  FILE *candidates = fopen(argv[3], "w");
  if (queries == NULL) {
    printf("Failed to open candidates file\n");
    return -1;
  }
  double total_time = 0.0;
  char q[256];
  while (fscanf(queries, "%s\n", q) != EOF) {
    // printf("Q is %s\n", q);
    fprintf(candidates, "%s ", q);

    MtiaCylinder *cyls[MAX_MINUTIAE_PER_FP];
    int read = readCylindersFromFile(q, cyls);
    if (read == -1) {
      fclose(queries);
      fclose(candidates);
      destroyHashTable(db);
      return -1;
    }

    clock_t t1 = clock();
    SRecord *res = search_template(db, cyls, read, k);
    clock_t t2 = clock();
    total_time += (double)(t2 - t1) / (CLOCKS_PER_SEC / 1000);

    for (int i = 0; i < k; i++) {
      // printf("AAAAAAAA %s\n",
      // db->template_map[res[i].template_index].template);
      fprintf(candidates, "%s ",
              db->template_map[res[i].template_index].template);
    }
    fprintf(candidates, "\n");

    for (int i = 0; i < read; i++) free(cyls[i]);
    free(res);
  }

  printf("%f ms\n", total_time);

  fclose(queries);
  fclose(candidates);
  destroyHashTable(db);
  return 0;
}
