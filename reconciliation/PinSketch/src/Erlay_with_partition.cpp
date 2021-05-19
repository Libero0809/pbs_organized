// Erlay.cpp
#include <cassert>
#include <cmath>
#include <cstdlib> /* for exit */
#include <cstring>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <minisketch.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <xxhash.h>

#include "FileReader.h"
#include "SimpleTimer.h"

using namespace std;

#define MAX_FILENAME_LEN 512
#define DEFAULT_PARAM_FILENAME_PREFIX "../perf/params_r"

int N_TEST = 0;
int DIFF_SIZE = 0;
int DIFF_START = -1;
int UNION_SIZE = 0;

char ESTDIFFSIZE_FILE[MAX_FILENAME_LEN];
char PARAM_FILENAME_PREFIX[MAX_FILENAME_LEN];
char INPUT_SETS_FILE[MAX_FILENAME_LEN];
bool USE_EST = false;
float EST_INFLATION_RATIO = -1.0;

const int KEY_SIZE = 4; // bytes
const int BYTE = 8;
const int SEED = 1023;
const int KEY_BITS = KEY_SIZE * BYTE;

int BCH_LENGTH = KEY_BITS;
int BCH_CAP = -1;
float GROUP_AVG = -1.0;
int FURTHER_DECOMP = 3;
int ROUNDS_FOR99 = 2;

#include <cstdio>
#ifdef DEBUG
#define debug(...) fprintf(stdout, __VA_ARGS__)
#else
#define debug(...)
#endif

void usage(const char *progname) {
  printf("Usage: %s -n NTEST -d DIFFSIZE -s DIFFSTART -u UNIONSIZE -i "
         "INPUTSETSFILENAME"
         "-m AVGDIFFINEACH -c FURTHERDECOMPOSENUM -R ROUNDFOR99 [-l BCHLENGTH "
         "-t BCHCAPACITY -p PARAMFILENAMEPREFIX] [-e ESTFILENAME -r "
         "ESTINFLATIONRATIO]\n",
         progname);
  exit(1);
}

using element_t = int;

inline std::vector<std::vector<element_t>>
partition(const vector<element_t> &set, int g, unsigned seed) {
  std::vector<std::vector<element_t>> subsets(g);
  for (auto &subset : subsets)
    subset.reserve(int(std::ceil(set.size() / 1.0 / g)));

  for (const auto &element : set) {
    unsigned gid = XXH32(&element, sizeof(element), seed) % g;
    subsets[gid].push_back(element);
  }
  return subsets;
}

unordered_map<int, pair<int /* bch length */, int /* bch decoding capacity */>>
loadBestParam(int round) {
  assert(round == 2 || round == 3);

  auto parameter_file = string(PARAM_FILENAME_PREFIX);
  if  (parameter_file.rfind(".csv") == std::string::npos)
       parameter_file += to_string(round) + ".csv";
  unordered_map<int, pair<int, int>> parameters{};

  string line;
  ifstream myfile(parameter_file);
  if (!myfile.is_open()) {
    std::cout << "Unable to open parameter file: " << parameter_file << "\n";
    exit(-1);
  }

  std::string results[4];

  while (getline(myfile, line)) {
    std::istringstream iss(line);
    int j = 0;
    while (std::getline(iss, results[j], ',')) {
      j++;
    }
    int expected_diff_size = std::stoi(results[0]);
    int bch_length = std::stoi(results[2]);
    int bch_capacity = std::stoi(results[3]);
    assert(parameters.count(expected_diff_size) == 0);
    parameters[expected_diff_size] = {bch_length, bch_capacity};
  }
  myfile.close();

  return parameters;
}

void arg_parse(int argc, char *argv[]) {
  int c;
  char *progname;
  char *p;

  progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

  memset(ESTDIFFSIZE_FILE, 0, MAX_FILENAME_LEN);
  memset(INPUT_SETS_FILE, 0, MAX_FILENAME_LEN);
  memset(PARAM_FILENAME_PREFIX, 0, MAX_FILENAME_LEN);
  while (true) {
    static struct option long_options[] = {
        {"ntest", required_argument, nullptr, 'n'},
        {"diffsize", required_argument, nullptr, 'd'},
        {"diffstart", required_argument, nullptr, 's'},
        {"unionsize", required_argument, nullptr, 'u'},
        {"avgdiffineach", required_argument, nullptr, 'm'},
        {"furtherdecomp", required_argument, nullptr, 'c'},
        {"roundfor99", required_argument, nullptr, 'R'},
        {"bchlength", optional_argument, nullptr, 'l'},
        {"bchcapacity", optional_argument, nullptr, 't'},
        {"estfilename", optional_argument, nullptr, 'e'},
        {"paramfilenameprefix", optional_argument, nullptr, 'p'},
        {"inputsetsfilename", required_argument, nullptr, 'i'},
        {"estinflationratio", optional_argument, nullptr, 'r'},
        {nullptr, 0, nullptr, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "n:d:s:u:m:c:R:l:t:e:r:p:i:", long_options,
                    &option_index);

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
    case 'm':
      debug("option -m with value `%s'\n", optarg);
      GROUP_AVG = atof(optarg);
      break;
    case 'c':
      debug("option -c with value `%s'\n", optarg);
      FURTHER_DECOMP = atoi(optarg);
      break;
    case 'l':
      debug("option -l with value `%s'\n", optarg);
      BCH_LENGTH = atoi(optarg);
      break;

    case 'R':
      debug("option -R with value `%s'\n", optarg);
      ROUNDS_FOR99 = atoi(optarg);
      break;

    case 't':
      debug("option -t with value `%s'\n", optarg);
      BCH_CAP = atoi(optarg);
      break;

    case 'e':
      debug("option -e with value `%s'\n", optarg);

      strcpy(ESTDIFFSIZE_FILE, optarg);

      USE_EST = true;

      break;
    case 'i':
      debug("option -i with value `%s'\n", optarg);

      strcpy(INPUT_SETS_FILE, optarg);

      break;
    case 'p':
      debug("option -p with value `%s'\n", optarg);

      strcpy(PARAM_FILENAME_PREFIX, optarg);

      break;
    case 'r':
      debug("option -r with value `%s'\n", optarg);

      EST_INFLATION_RATIO = atof(optarg);

      break;

    case '?':
      usage(progname);

      break;

    default:
      usage(progname);
    }
  }

  if (strlen(PARAM_FILENAME_PREFIX) == 0) {
    strcpy(PARAM_FILENAME_PREFIX, DEFAULT_PARAM_FILENAME_PREFIX);
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
                  to_string(USE_EST) + "_" + to_string(N_TEST) + "_" +
                  to_string(ROUNDS_FOR99) + "_ErlayWP.txt";

  fout.open(filename);
  if (!fout.is_open()) {
    fprintf(stderr, "Failed to open %s\n", filename.c_str());
    exit(1);
  }

  fout << setw(15) << "ntest" << setw(15) << N_TEST << endl;
  fout << setw(15) << "unionsize" << setw(15) << UNION_SIZE << endl;
  fout << setw(15) << "diffsize" << setw(15) << DIFF_SIZE << endl;
  fout << setw(15) << "useest" << setw(15) << USE_EST << endl;
  fout << setw(15) << "groupavg" << setw(15) << GROUP_AVG << endl;
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

  // load best parameters
  unordered_map<int, pair<int, int>> best_params = loadBestParam(ROUNDS_FOR99);

  fout << "#tid,round,succeed,transmitted_bytes,num_recovered,encoding_time,"
          "decoding_time,"
          "true_diff,key_bits,capacity,bch_bits,notified_bits,failed_groups\n";

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
      t = int(ceil(static_cast<double>(diff_est) * EST_INFLATION_RATIO));
    }

#ifdef CLION_DEBUG
    if (BCH_CAP == -1) {
#endif
      assert(best_params.count(t) > 0);
      auto param_pair = best_params[t];
      BCH_CAP = param_pair.second;
#ifdef CLION_DEBUG
    }
#endif

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
    int num_subsets = int(std::ceil(t / GROUP_AVG));
    auto subsetA = partition(setA, num_subsets, SEED);
    auto subsetB = partition(setB, num_subsets, SEED);
    t_encoding = timer.elapsed();
    t_decoding = 0;

    int round = 0;
    int g_num = num_subsets;

    while (g_num > 0) {

      int num_recovered = 0;
      std::vector<int> failed;
      correct = true;

      for (int g = 0; g < g_num; ++g) {
        timer.restart();
        minisketch *sketch_a = minisketch_create(BCH_LENGTH, 0, BCH_CAP);
        minisketch *sketch_b = minisketch_create(BCH_LENGTH, 0, BCH_CAP);
        for (const auto x : subsetA[g]) {
          minisketch_add_uint64(sketch_a, x);
        }
        for (const auto y : subsetB[g]) {
          minisketch_add_uint64(sketch_b, y);
        }
        t_encoding += timer.elapsed();

        timer.restart();
        vector<uint64_t> pos(BCH_CAP);
        minisketch_merge(sketch_b, sketch_a);
        int p = minisketch_decode(sketch_b, BCH_CAP, &pos[0]);
        t_decoding += timer.elapsed();

        minisketch_destroy(sketch_a);
        minisketch_destroy(sketch_b);

        if (p == -1) {
          failed.push_back(g);
        } else {
          num_recovered += p;

          for (int jj = 0; jj < p; ++jj) {
            if (!diff.count(pos[jj])) {
              correct = false;
              break;
            }
          }
        }
      }

      auto bch_bits = (BCH_LENGTH * BCH_CAP * g_num);
      auto notified_bits = (failed.size() * int(ceil(log2(g_num))));

      ++round;
      if (failed.empty()) {
        fout << (i + 1) << "," << round << "," << (correct ? 1 : -1) << ","
             << (bch_bits + notified_bits) << "," << num_recovered << ","
             << fixed << t_encoding << "," << fixed << t_decoding << ","
             << DIFF_SIZE << "," << BCH_LENGTH << "," << BCH_CAP << ","
             << bch_bits << "," << notified_bits << "," << failed.size()
             << "\n";
      } else {
        fout << (i + 1) << "," << round << "," << 0 << ","
             << (bch_bits + notified_bits) << "," << num_recovered << ","
             << fixed << t_encoding << "," << fixed << t_decoding << ","
             << DIFF_SIZE << "," << BCH_LENGTH << "," << BCH_CAP << ","
             << bch_bits << "," << notified_bits << "," << failed.size()
             << "\n";
      }

      if (!failed.empty()) {
        timer.restart();
        vector<vector<element_t>> tmp_subsetA, tmp_subsetB;
        for (const auto &gid : failed) {
          auto tmpA = partition(subsetA[gid], FURTHER_DECOMP, SEED + round);
          tmp_subsetA.insert(tmp_subsetA.end(), tmpA.begin(), tmpA.end());
          auto tmpB = partition(subsetB[gid], FURTHER_DECOMP, SEED + round);
          tmp_subsetB.insert(tmp_subsetB.end(), tmpB.begin(), tmpB.end());
        }
        t_encoding = timer.elapsed();
        g_num = tmp_subsetA.size();

        assert((size_t)g_num == failed.size() * FURTHER_DECOMP);

        subsetA = std::move(tmp_subsetA);
        subsetB = std::move(tmp_subsetB);
      } else {
        t_encoding = 0;
        g_num = 0;
      }
      t_decoding = 0;
    }

    if ((i + 1) % 100 == 0) {
      fprintf(stdout, "%d/%d\n", i + 1, n_test);
    }
  }

  if (USE_EST)
    fest.close();
  fout.close();
}
