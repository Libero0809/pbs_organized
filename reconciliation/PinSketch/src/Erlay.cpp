// Erlay.cpp
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <minisketch.h>
#include <string>
#include <unordered_set>
#include <vector>
#include <xxhash.h>

#include <cstdlib> /* for exit */
#include <getopt.h>

#include "FileReader.h"
#include "SimpleTimer.h"

using namespace std;

#define MAX_FILENAME_LEN 512

int N_TEST = 0;
int DIFF_SIZE = 0;
int DIFF_START = -1;
int UNION_SIZE = 0;

char ESTDIFFSIZE_FILE[MAX_FILENAME_LEN];
char INPUT_SETS_FILE[MAX_FILENAME_LEN];
bool USE_EST = false;
float EST_INFLATION_RATIO = -1.0;

const int KEY_SIZE = 4; // bytes
const int BYTE = 8;
const int SEED = 1023;
const int KEY_BITS = KEY_SIZE * BYTE;

#include <cstdio>
#ifdef DEBUG
#define debug(...) fprintf(stdout, __VA_ARGS__)
#else
#define debug(...)
#endif

void usage(const char *progname) {
  printf("Usage: %s -n NTEST -d DIFFSIZE -s DIFFSTART -u UNIONSIZE -e "
         "ESTFILENAME -i INPUTSETSFILENAME [-r ESTINFLATIONRATIO]\n",
         progname);
  exit(1);
}

void arg_parse(int argc, char *argv[]) {
  int c;
  char *progname;
  char *p;

  progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

  memset(ESTDIFFSIZE_FILE, 0, MAX_FILENAME_LEN);
  memset(INPUT_SETS_FILE, 0, MAX_FILENAME_LEN);
  while (true) {
    static struct option long_options[] = {
        {"ntest", required_argument, nullptr, 'n'},
        {"diffsize", required_argument, nullptr, 'd'},
        {"diffstart", required_argument, nullptr, 's'},
        {"unionsize", required_argument, nullptr, 'u'},
        {"estfilename", required_argument, nullptr, 'e'},
        {"inputsetsfilename", required_argument, nullptr, 'i'},
        {"estinflationratio", optional_argument, nullptr, 'r'},
        {nullptr, 0, nullptr, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "n:d:s:u:e:r:i:h", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) {
      break;
    }

    switch (c) {
    case 'n':
      debug("option -n with value `%s'\n", optarg);
      N_TEST = atoi(optarg);
      break;

    case 'd':
      debug("option -d with value `%s'\n", optarg);
      DIFF_SIZE = atoi(optarg);
      break;

    case 's':
      debug("option -s with value `%s'\n", optarg);
      DIFF_START = atoi(optarg);
      break;

    case 'u':
      debug("option -u with value `%s'\n", optarg);
      UNION_SIZE = atoi(optarg);
      break;

    case 'e':
      debug("option -e with value `%s'\n", optarg);

      strcpy(ESTDIFFSIZE_FILE, optarg);

      USE_EST = true;

      break;
    case 'r':
      debug("option -r with value `%s'\n", optarg);

      EST_INFLATION_RATIO = atof(optarg);

      break;

    case 'i':
      debug("option -i with value `%s'\n", optarg);
      strcpy(INPUT_SETS_FILE, optarg);
      break;

    default:
      usage(progname);
    }
  }

  if (EST_INFLATION_RATIO > 0) {
    assert(EST_INFLATION_RATIO >= 1.0);
    assert(USE_EST);
  }

  if (UNION_SIZE <= 0 || DIFF_SIZE <= 0 || N_TEST < 0 || DIFF_START == -1) {
    usage(progname);
  }
}

int main(int argc, char *argv[]) {
  arg_parse(argc, argv);

  ofstream fout;

  auto filename = to_string(UNION_SIZE) + "_" + to_string(DIFF_SIZE) + "_" +
                  to_string(USE_EST) + "_" + to_string(N_TEST) + "_Erlay.txt";
  fout.open(filename);
  if (!fout.is_open()) {
    fprintf(stderr, "Failed to open %s\n", filename.c_str());
    exit(1);
  }

  fout << setw(15) << "ntest" << setw(15) << N_TEST << endl;
  fout << setw(15) << "unionsize" << setw(15) << UNION_SIZE << endl;
  fout << setw(15) << "diffsize" << setw(15) << DIFF_SIZE << endl;
  fout << setw(15) << "useest" << setw(15) << USE_EST << endl;
  fout << "# ====================================================\n";

  TxTFileReader reader(INPUT_SETS_FILE);
  reader.skip_lines(3); // skip 3 lines

  int lg_j, lg_k, lg_m;

  int diff_size = DIFF_SIZE;
  int diff_start = DIFF_START;
  int union_size = UNION_SIZE;
  int n_test = N_TEST;

  int t = DIFF_SIZE;

  float diff_est = -1.0;
  bool correct;

  std::vector<int> setA, setB;
  setB.resize(union_size - diff_size);

  unordered_set<int> diff; // needed when checking whether successfully decoded

  std::ifstream fest;

  if (USE_EST) {
    fest.open(ESTDIFFSIZE_FILE);
    if (!fest.is_open()) {
      fprintf(stderr, "Failed to open %s\n", ESTDIFFSIZE_FILE);
      exit(1);
    }
  }

  fout << "#tid,round,transmitted_bytes,succeed,encoding_time,decoding_time,"
          "true_diff,key_bits,capacity\n";

  SimpleTimer timer;
  double t_encoding, t_decoding;

  for (int i = 0; i < n_test; ++i) {
    // read the union
    reader.read<int>(setA, union_size);

    if (USE_EST) {
      // In the case where the ground truth value of the size
      // of the set difference is not known.
      fest >> diff_est;
      debug("Get estimate %.6f\n", diff_est);
      t = int(ceil(diff_est * EST_INFLATION_RATIO));
    }

    // copy elements to set B
    for (lg_j = 0, lg_k = 0; lg_j < diff_start; ++lg_j, ++lg_k) {
      setB[lg_k] = setA[lg_j];
    }

    // copy true set difference
    diff.clear();
    for (lg_m = 0; lg_m < diff_size; ++lg_m, ++lg_j) {
      diff.insert(setA[lg_j]);
    }

    // copy elements to set B
    for (; lg_j < union_size; ++lg_j, ++lg_k) {
      setB[lg_k] = setA[lg_j];
    }

    // ensure size of set B is correct
    assert((size_t)lg_k == setB.size());

    timer.restart();
    minisketch *sketch_a = minisketch_create(KEY_BITS, 0, t);
    minisketch *sketch_b = minisketch_create(KEY_BITS, 0, t);
    for (const auto x : setA) {
      minisketch_add_uint64(sketch_a, x);
    }
    for (const auto y : setB) {
      minisketch_add_uint64(sketch_b, y);
    }
    t_encoding = timer.elapsed();

    vector<uint64_t> pos(t);

    timer.restart();
    minisketch_merge(sketch_b, sketch_a);
    int p = minisketch_decode(sketch_b, t, &pos[0]);
    t_decoding = timer.elapsed();

    if (p == -1) {
      // failed for the first round
      fout << (i + 1) << ",1," << KEY_SIZE * t << "," << 0 << "," << fixed
           << t_encoding << "," << fixed << t_decoding << "," << DIFF_SIZE
           << "," << KEY_BITS << "," << t << "\n";

      // start the second round
      timer.restart();
      minisketch *sketch_a1 = minisketch_create(KEY_BITS, 0, t);
      minisketch *sketch_b1 = minisketch_create(KEY_BITS, 0, t);
      for (const auto x : setA) {
        if (XXH32(&x, sizeof(x), SEED) % 2 == 0) {
          minisketch_add_uint64(sketch_a1, x);
        }
      }
      for (const auto y : setB) {
        if (XXH32(&y, sizeof(y), SEED) % 2 == 0) {
          minisketch_add_uint64(sketch_b1, y);
        }
      }
      t_encoding = timer.elapsed();

      vector<uint64_t> pos1(t);

      timer.restart();
      minisketch_merge(sketch_b1, sketch_a1);
      int p1 = minisketch_decode(sketch_b1, t, &pos1[0]);
      t_decoding = timer.elapsed();

      if (p1 == -1) {
        // failed
        fout << (i + 1) << ",2," << KEY_SIZE * t << "," << 0 << "," << fixed
             << t_encoding << "," << fixed << t_decoding << "," << DIFF_SIZE
             << "," << KEY_BITS << "," << t << "\n";
      } else {
        // succeeded
        correct = true;
        for (int jj = 0; jj < p1; ++jj) {
          if (diff.count(pos1[jj]) > 0) {
            diff.erase(pos1[jj]);
          } else {
            correct = false;
            break;
          }
        }

        timer.restart();
        for (int jj = 0; jj < p1; ++jj) {
          // not sure whether we should use the untouched copy of
          // sketch_b or not
          minisketch_add_uint64(sketch_b, pos1[jj]);
        }
        int p2 = minisketch_decode(sketch_b, t, &pos[0]);
        t_decoding = +timer.elapsed();

        if (correct && p2 != -1) {
          for (int jj = 0; jj < p2; ++jj) {
            if (diff.count(pos[jj]) > 0) {
              diff.erase(pos[jj]);
            } else {
              correct = false;
              break;
            }
          }
          if (!diff.empty()) {
            correct = false;
          }
        }

        if (p2 == -1 || (!correct)) {
          // failed
          fout << (i + 1) << ",2," << KEY_SIZE * t << "," << (correct ? 0 : -1)
               << "," << fixed << t_encoding << "," << fixed << t_decoding
               << "," << DIFF_SIZE << "," << KEY_BITS << "," << t << "\n";
        } else {
          // succeeded
          fout << (i + 1) << ",2," << KEY_SIZE * t << "," << 1 << "," << fixed
               << t_encoding << "," << fixed << t_decoding << "," << DIFF_SIZE
               << "," << KEY_BITS << "," << t << "\n";
        }
      }

      minisketch_destroy(sketch_a1);
      minisketch_destroy(sketch_b1);

    } else {
      correct = true;

      if ((size_t)p != diff.size()) {
        correct = false;
      } else {
        for (int jj = 0; jj < p; ++jj) {
          if (diff.count(pos[jj]) == 0) {
            correct = false;
            break;
          }
        }
      }

      if (correct) {
        fout << (i + 1) << ",1," << KEY_SIZE * t << "," << 1 << "," << fixed
             << t_encoding << "," << fixed << t_decoding << "," << DIFF_SIZE
             << "," << KEY_BITS << "," << t << "\n";
      } else {
        fout << (i + 1) << ",1," << KEY_SIZE * t << "," << -1 << "," << fixed
             << t_encoding << "," << fixed << t_decoding << "," << DIFF_SIZE
             << "," << KEY_BITS << "," << t << "\n";
      }
    }
    minisketch_destroy(sketch_a);
    minisketch_destroy(sketch_b);

    if ((i + 1) % 100 == 0) {
      fprintf(stdout, "%d/%d\n", i + 1, n_test);
    }
  }

  if (USE_EST)
    fest.close();
  fout.close();
}
