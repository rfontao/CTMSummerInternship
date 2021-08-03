#include "tables.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>

// https://stackoverflow.com/questions/51387998/count-bits-1-on-an-integer-as-fast-as-gcc-builtin-popcountint
int bitcount[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

int countBit1Fast(int n) {
  int i, count = 0;
  unsigned char* ptr = (unsigned char*)&n;
  for (i = 0; i < sizeof(int); i++) {
    count += bitcount[ptr[i]];
  }
  return count;
}

static int gen_masks(MultiHashTable* ht) {
  ht->masks = (uint64_t*)malloc(ht->m * sizeof(uint64_t));
  if (ht->masks == NULL) {
    return -1;
  }

  int cur_index = 0;
  for (int i = 0; i < ht->m; ++i) {
    ht->tables[i].actual_index = cur_index;
    int cur_index_32 = cur_index / 32;

    if (((cur_index + ht->tables[i].bit_amount) % 32) > cur_index_32 &&
        !((cur_index + ht->tables[i].bit_amount) < 32)) {  // Splits ints

      uint64_t mask = 0ull;
      for (uint64_t j = cur_index; j < cur_index + ht->tables[i].bit_amount;
           ++j) {
        mask |= (1ULL << j);
      }
      ht->masks[i] = mask;

    } else {
      uint32_t mask = 0;
      for (size_t j = cur_index; j < cur_index + ht->tables[i].bit_amount;
           ++j) {
        mask |= (1ULL << j);
      }

      ht->masks[i] = (uint64_t)mask;
    }
    cur_index += ht->tables[i].bit_amount;
  }

  return 0;
}

MultiHashTable* initHashTable(int min_bit_size, int m, size_t template_amount) {
  MultiHashTable* ret = (MultiHashTable*)malloc(sizeof(MultiHashTable));
  if (ret == NULL) {
    return NULL;
  }
  ret->m = m;

  ret->template_map_len = template_amount;
  ret->template_map = (Template*)malloc(template_amount * sizeof(Template));
  if (ret->template_map == NULL) {
    free(ret);
    return NULL;
  }

  ret->master_list_len = template_amount * MAX_MINUTIAE_PER_FP;
  ret->master_list =
      (MtiaCylinder**)malloc(ret->master_list_len * sizeof(MtiaCylinder*));
  if (ret->master_list == NULL) {
    free(ret->template_map);
    free(ret);
    return NULL;
  }

  // Allocating "array of hash tables"
  ret->tables_len = m;
  ret->tables = (HashTable*)malloc(sizeof(HashTable) * m);
  if (ret->tables == NULL) {
    free(ret->template_map);
    free(ret->master_list);
    free(ret);
    return NULL;
  }

  // Allocating each hash table

  // If value is bigger than x.5 all initial values minus the last one is
  // rounded up and last is rounded down Else last one is rounded up and the
  // others rounded down
  double table_size = (min_bit_size / (double)m);
  int last_round_up;
  if ((table_size - (int)table_size) < 0.5) {
    last_round_up = 1;
  } else {
    last_round_up = 0;
  }

  for (int i = 0; i < m; ++i) {
    if (i != m - 1) {
      ret->tables[i].bit_amount = (int)round((min_bit_size / (double)m));
      // printf("Len: %d", (int)round((min_bit_size / (double)m)));
    } else {  // Last iteration
      if (last_round_up) {
        ret->tables[i].bit_amount = (int)(0.5 + (min_bit_size / (double)m));
      } else {
        ret->tables[i].bit_amount = min_bit_size / m;
      }
    }
    ret->tables[i].len = pow(2, ret->tables[i].bit_amount);

    // printf("Table %d size: %lu\n", i, ret->tables[i].len);
    ret->tables[i].buckets =
        (Bucket*)malloc(sizeof(Bucket) * ret->tables[i].len);
    if (ret->tables[i].buckets == NULL) {
      destroyHashTable(ret);
      return NULL;
    }

    for (size_t j = 0; j < ret->tables[i].len; ++j) {
      ret->tables[i].buckets[j].curr_size = 0;
    }

    ret->tables[i].filled_buckets_list_len = 0;
    ret->tables[i].filled_buckets_list =
        (int*)malloc(ret->tables[i].len * sizeof(int));
    if (ret->tables[i].filled_buckets_list == NULL) {
      destroyHashTable(ret);
      return NULL;
    }
  }

  // TODO prealocar em cada bucket?

  if (gen_masks(ret) < 0) {
    destroyHashTable(ret);
    return NULL;
  }

  return ret;
}

// returns the amount of candidates
int search_mtia(MultiHashTable* ht, MtiaCylinder* cyl, int k,
                SearchResult** out) {
  int r1 = 0, a = 0, r = 0;
  int result_count = 0;
  // 2 * k for safety
  // Will be realloced as needed
  *out = (SearchResult*)malloc(sizeof(SearchResult));
  if (*out == NULL) {
    fprintf(stderr, "Failed to allocate output array\n");
    return -1;
  }

  do {
    //?
    r = ht->m * r1 + a;

    int value = get_substring_bits(ht, cyl, a);
    // for (int i = 0; i < ht->tables[a].len; ++i) {
    for (int j = 0; j < ht->tables[a].filled_buckets_list_len; ++j) {
      int i = ht->tables[a].filled_buckets_list[j];

      // int part_distance = pop_count(i ^ value);
      // printf("Value: %d\n", value);
      int part_distance = countBit1Fast(i ^ value);
      if (part_distance == r1) {
        *out = (SearchResult*)realloc(
            *out, (result_count + ht->tables[a].buckets[i].curr_size) *
                      sizeof(SearchResult));
        if (out == NULL) {
          fprintf(stderr, "Failed to reallocate output array\n");
          return -1;
        }

        for (size_t j = 0; j < ht->tables[a].buckets[i].curr_size; ++j) {
          SearchResult res;
          res.cyl = ht->tables[a].buckets[i].content[j];
          // res.distance =
          //     hamming_distance(ht->tables[a].buckets[i].content[j], cyl);
          (*out)[result_count++] = res;
        }
      }
    }

    a++;
    if (a >= ht->m) {
      a = 0;
      r1++;
    }
    r++;
    // printf("Radius: %d Radius': %d\n", r, r1);

  } while (result_count < k);

  // printf("-----------\n");
  return result_count;
}

// TODO assert size < 64 ou < 32
int addCylinder(MultiHashTable* ht, MtiaCylinder* cyl) {
  static int master_list_index = 0;
  for (size_t i = 0; i < ht->tables_len; ++i) {
    int value = get_substring_bits(ht, cyl, i);

    // TODO Needs to be changed probably
    ht->tables[i].buckets[value].content = (MtiaCylinder**)realloc(
        ht->tables[i].buckets[value].content,
        sizeof(MtiaCylinder*) * (ht->tables[i].buckets[value].curr_size + 1));

    if (ht->tables[i].buckets[value].content == NULL) {
      fprintf(stderr, "Couldnt reallocate array of bucket %d of table %lu",
              value, i);
      return -1;
    }

    ht->tables[i]
        .buckets[value]
        .content[ht->tables[i].buckets[value].curr_size] = cyl;
    ht->tables[i].buckets[value].curr_size++;

    // NOT IDEAL
    int flag = 0;
    for (size_t j = 0; j < ht->tables[i].filled_buckets_list_len; j++) {
      if (ht->tables[i].filled_buckets_list[j] == value) {
        flag = 1;
        break;
      }
    }

    if (!flag) {
      ht->tables[i].filled_buckets_list[ht->tables[i].filled_buckets_list_len] =
          value;
      ht->tables[i].filled_buckets_list_len++;
    }
  }

  ht->master_list[master_list_index] = cyl;
  master_list_index++;
  // printf("MLI: %d\n", master_list_index);

  return 0;
}

// Check ifs
void destroyHashTable(MultiHashTable* hash_table) {
  // Freeing contents
  for (size_t i = 0; i < hash_table->master_list_len; ++i) {
    if (hash_table->master_list[i] != NULL) free(hash_table->master_list[i]);
  }

  for (size_t i = 0; i < hash_table->template_map_len; ++i) {
    if (hash_table->template_map[i].template != NULL)
      free(hash_table->template_map[i].template);
  }

  // Freeing each bucket
  for (size_t i = 0; i < hash_table->tables_len; i++) {
    for (size_t j = 0; j < hash_table->tables[i].len; j++) {
      if (hash_table->tables[i].buckets[j].content != NULL)
        free(hash_table->tables[i].buckets[j].content);
    }
    if (hash_table->tables[i].filled_buckets_list != NULL)
      free(hash_table->tables[i].filled_buckets_list);
    if (hash_table->tables[i].buckets != NULL)
      free(hash_table->tables[i].buckets);
  }

  if (hash_table->tables != NULL) free(hash_table->tables);
  if (hash_table->master_list != NULL) free(hash_table->master_list);
  if (hash_table->masks != NULL) free(hash_table->masks);
  if (hash_table->template_map != NULL) free(hash_table->template_map);
  if (hash_table != NULL) free(hash_table);
}

int compare(const void* p1, const void* p2) {
  SRecord* s1 = (SRecord*)p1;
  SRecord* s2 = (SRecord*)p2;

  if (s1->score < s2->score)
    return 1;
  else if (s1->score == s2->score)
    return 0;
  else
    return -1;
  /* or simply: return i1 - i2; */
}

SRecord* search_template(MultiHashTable* ht, MtiaCylinder** q, int cyl_count,
                         int k) {
  SRecord* S = (SRecord*)calloc(ht->template_map_len, sizeof(SRecord));
  if (S == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < ht->template_map_len; ++i)
    S[i].template_index = (double)i;

  double* A = (double*)calloc(ht->template_map_len * MAX_MINUTIAE_PER_FP,
                              sizeof(double));
  if (A == NULL) {
    free(S);
    return NULL;
  }

  int* collisions = (int*)calloc(ht->template_map_len, sizeof(int));
  if (collisions == NULL) {
    free(S);
    return NULL;
  }

  for (int i = 0; i < cyl_count; ++i) {
    MtiaCylinder* cyl = q[i];
    SearchResult* results = NULL;
    int result_amount = search_mtia(ht, cyl, k, &results);
    if (result_amount < 0) {
      free(S);
      free(results);
      destroyHashTable(ht);
      return NULL;
    }

    // First element checks if there are collisions

    for (size_t j = 0; j < result_amount; ++j) {
      if (compatible(q[i], results[j].cyl, MAX_DIST, MAX_ANGLE)) {
        collisions[results[j].cyl->template_index] = 1;
        A[results[j].cyl->template_index * MAX_MINUTIAE_PER_FP +
          results[j].cyl->minutiae_index] = sim(q[i], results[j].cyl);
      }
    }

    for (size_t j = 0; j < ht->template_map_len; ++j) {
      if (collisions[j] == 0) {
        continue;
      }

      double CFmax = 0;
      for (size_t k = 0; k < MAX_MINUTIAE_PER_FP; ++k) {
        if (A[j * MAX_MINUTIAE_PER_FP + k] > CFmax) {
          CFmax = A[j * MAX_MINUTIAE_PER_FP + k];
        }
      }

      S[j].score = S[j].score + CFmax;
    }

    memset(collisions, 0, ht->template_map_len * sizeof(int));
    memset(A, 0, ht->template_map_len * MAX_MINUTIAE_PER_FP * sizeof(double));
    free(results);
  }
  free(A);
  free(collisions);

  for (size_t i = 0; i < ht->template_map_len; ++i) {
    S[i].score /= cyl_count;
  }

  qsort(S, ht->template_map_len, sizeof(SRecord), compare);

  return S;
}

inline int compatible(MtiaCylinder* q, MtiaCylinder* t, double max_squared_dist,
                      int max_angle) {
  return minutiae_squared_distance(&(q->minutia), &(t->minutia)) <
             max_squared_dist &&
         angle_minimum_difference(q->minutia.angle, t->minutia.angle) <
             max_angle;
}

double sim(MtiaCylinder* q, MtiaCylinder* t) {
  return 1.0 - (hamming_distance(q, t) / (double)MCC_VALUE_SIZE);
}

int hamming_distance(MtiaCylinder* q, MtiaCylinder* t) {
  int count = 0;
  for (int i = 0; i < MCC_VALUE_SIZE; i++)
    count += countBit1Fast(q->values[i] ^ t->values[i]);
  return count;
}

int get_substring_bits(MultiHashTable* ht, MtiaCylinder* cyl, int index) {
  // int cur_index = index * ht->tables[0].bit_amount;
  int cur_index = ht->tables[index].actual_index;

  // printf("Saved index: %x   Fast index: %lx\n",
  // ht->tables[index].actual_index,
  //  index * ht->tables[0].bit_amount);
  int cur_index_32 = cur_index / 32;
  if (((cur_index + ht->tables[index].bit_amount) % 32) > cur_index_32 &&
      !((cur_index + ht->tables[index].bit_amount) < 32)) {  // Splits ints

    // Join two ints
    uint64_t joint_ints = 0ull;
    joint_ints |= cyl->values[cur_index_32];
    joint_ints <<= 32;
    joint_ints |=
        cyl->values[cur_index_32 + 1];  // Shouldn't give index out of range
                                        // if everything is correct

    return (joint_ints & ht->masks[index]) >> (cur_index - cur_index_32 * 32);

  } else {
    return (cyl->values[cur_index_32] & ht->masks[index]) >>
           (cur_index - cur_index_32 * 32);
  }
}