#include "io.h"

int main(int argc, char const* argv[]) {
  if (argc != 3) {
    printf("Usage: %s inputFile1 inputFile2\n", argv[0]);
    return -1;
  }

  float score = (float)readMatchScoreTemplates(argv[1], argv[2]);
  if (score < 0) {
    return -1;
  }
  printf("The comparison score of the two fingerprints is %f\n", score);

  return 0;
}
