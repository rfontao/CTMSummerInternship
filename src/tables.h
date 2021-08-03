#ifndef _MULTI_HASH_TABLE_
#define _MULTI_HASH_TABLE_

#include <stdint.h>

#define MAX_MINUTIAE_PER_FP 256
#define PI 3.141592653589793238462643383280
// #define MAX_ANGLE PI / 4.0
#define MAX_ANGLE 9
// #define MAX_DIST 5000
#define MAX_DIST 256 * 256

typedef struct {
  MtiaCylinder** content;
  size_t curr_size;
} Bucket;

typedef struct {
  Bucket* buckets;
  size_t len;
  size_t bit_amount;
  int* filled_buckets_list;
  size_t filled_buckets_list_len;
  int actual_index;
} HashTable;

typedef struct {
  char* template;
  int min_amount;
} Template;

typedef struct {
  HashTable* tables;
  uint16_t tables_len;
  MtiaCylinder** master_list;  // Used on the process of freeing all elements
                               // (elements are repeated in the hash tables)
  size_t master_list_len;
  int m;
  Template* template_map;
  size_t template_map_len;
  uint64_t* masks;
} MultiHashTable;

typedef struct {
  MtiaCylinder* cyl;
  int distance;
} SearchResult;

typedef struct {
  int template_index;
  double score;
} SRecord;

MultiHashTable* initHashTable(int min_bit_size, int m, size_t master_list_len);
int addCylinder(MultiHashTable* ht, MtiaCylinder* cyl);
void destroyHashTable(MultiHashTable* hash_table);
int search_mtia(MultiHashTable* ht, MtiaCylinder* cyl, int k,
                SearchResult** out);
SRecord* search_template(MultiHashTable* ht, MtiaCylinder** q, int cyl_count,
                         int k);
double sim(MtiaCylinder* q, MtiaCylinder* t);
int hamming_distance(MtiaCylinder* q, MtiaCylinder* t);
int compatible(MtiaCylinder* q, MtiaCylinder* t, double min_dist,
               int min_angle);
int get_substring_bits(MultiHashTable* ht, MtiaCylinder* cyl, int index);

#endif /* _MULTI_HASH_TABLE_ */
