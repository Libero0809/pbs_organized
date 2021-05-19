#include <getopt.h>

#include <cstdlib>
#include <cstring>
#include <iomanip>

#include "FileReader.h"
#include "tow.h"
#include "xxhash_wrapper.h"

int DIFF_SIZE = 0;
int DIFF_START = -1;
int NUM_SKETCH = -1;
unsigned SEED = 0;
int KEY_BITS = 32;

#include <stdio.h>
#ifdef DEBUG
#define debug(...) fprintf(stdout, __VA_ARGS__)
#else
#define debug(...)
#endif

void usage(const char *progname) {
  printf("Usage: %s -d DIFFSIZE -s DIFFSTART -n NUMSKETCH [-S SEED]\n",
         progname);
  exit(1);
}

void arg_parse(int argc, char *argv[]) {
  int c;
  char *progname;
  char *p;

  progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

  while (true) {
    static struct option long_options[] = {
        {"diffsize", required_argument, nullptr, 'd'},
        {"diffstart", required_argument, nullptr, 's'},
        {"numsketch", required_argument, nullptr, 'n'},
        {"seed", optional_argument, nullptr, 'S'},
        {nullptr, 0, nullptr, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "d:s:n:S:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) break;

    switch (c) {
      case 'd':
        debug("option -d with value `%s'\n", optarg);
        DIFF_SIZE = atoi(optarg);
        break;

      case 's':
        debug("option -s with value `%s'\n", optarg);
        DIFF_START = atoi(optarg);
        break;

      case 'n':
        debug("option -n with value `%s'\n", optarg);
        NUM_SKETCH = atoi(optarg);
        break;

      case 'S':
        debug("option -S with value `%s'\n", optarg);

        SEED = atoi(optarg);
        break;

      case '?':
        usage(progname);

        break;

      default:
        usage(progname);
    }
  }

  if (DIFF_SIZE <= 0 || DIFF_START == -1 || NUM_SKETCH <= 0) {
    usage(progname);
  }

  if (SEED == 0)
    SEED = std::chrono::system_clock::now().time_since_epoch().count();
}

template <typename KEY_TYPE>
double ToW32(const std::vector<KEY_TYPE> &setA,
             const std::vector<KEY_TYPE> &setB, int num_sketches,
             unsigned seed) {
  static_assert(sizeof(KEY_TYPE) <= 4,
                "ToW32 only support key types with less than 32 bits");
  TugOfWarHash<XXHash> tow(num_sketches, seed);

  auto sketchA = tow.template apply<KEY_TYPE>(setA);
  auto sketchB = tow.template apply<KEY_TYPE>(setB);

  double d = 0.0, tmp;

  for (size_t i = 0; i < sketchA.size(); ++i) {
    tmp = sketchA[i] - sketchB[i];
    d += tmp * tmp;
  }

  return d / num_sketches;
}

/*
template <typename KEY_TYPE>
double ToW64(const std::vector<KEY_TYPE> &setA,
             const std::vector<KEY_TYPE> &setB, int num_sketches,
             unsigned seed) {
  static_assert(sizeof(KEY_TYPE) <= 8,
                "ToW32 only support key types with less than 64 bits");
  TugOfWarHash<XXHash64> tow(num_sketches, seed);

  auto sketchA = tow.template apply<KEY_TYPE>(setA);
  auto sketchB = tow.template apply<KEY_TYPE>(setB);

  double d = 0.0, tmp;

  for (size_t i = 0; i < sketchA.size(); ++i) {
    tmp = sketchA[i] - sketchB[i];
    d += tmp * tmp;
  }

  return d / num_sketches;
}
*/

int main(int argc, char *argv[]) {
  arg_parse(argc, argv);

  std::ofstream fout;

  auto out_filename =
      "../reconciliation/test-sets/1000000-1000/" +
      std::string("diff_estimates_with_tow") + "_" + std::to_string(DIFF_SIZE) +
      "_" + std::to_string(NUM_SKETCH) + "_" + std::to_string(SEED) + ".txt";

  fout.open(out_filename);

  if (!fout.is_open()) {
    fprintf(stderr, "Failed to open %s\n", out_filename.c_str());
    exit(1);
  }

  // TxTFileReader reader("../../test-sets/sets_100000_1000_32_1574465740.txt");
  TxTFileReader reader(
      "../reconciliation/test-sets/1000000-1000/"
      "sets_1000000_1000_32_1574465740.txt");
  // TxTFileReader reader(
  //     "/home/saber/Dropbox/large-files/sets_1000000_1000_32_1574465740.txt");

  reader.skip_lines(3);  // skip 3 lines

  int lg_m, lg_j;
  int union_size = 1000000;  // 100000;
  int num_sets = 1000;

  std::vector<int> setU, setDiff(DIFF_SIZE);

  for (int i = 0; i < num_sets; ++i) {
    reader.read<int>(setU, union_size);
    for (lg_m = 0, lg_j = DIFF_START; lg_m < DIFF_SIZE; ++lg_m, ++lg_j) {
      setDiff[lg_m] = setU[lg_j];
    }

    std::vector<int> setB;

    auto estimate = ToW32<int>(setDiff, setB, NUM_SKETCH, SEED);
    fout << std::fixed << estimate << "\n";
  }

  fout.close();
}
