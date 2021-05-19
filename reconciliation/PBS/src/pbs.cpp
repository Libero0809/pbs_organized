#include <getopt.h>
#include <minisketch.h>
#include <xxhash.h>

#include <cassert>
#include <cmath>
#include <cstdlib> /* for exit */
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "FileReader.h"
#include "SimpleTimer.h"

#ifdef USE_PY_SEEDS
// using the same seeds to make sure reproducibility
#include "seedpool.h"
#else
#include "random.h"
#endif

using namespace std;

// for ONLY printing messages when debugging
#ifdef DEBUG
#define debug(...) fprintf(stdout, __VA_ARGS__)
#else
#define debug(...)
#endif

void usage(const char *progname) {
  fprintf(stderr,
          "Usage: %s -n NTEST -d DIFFSIZE -s DIFFSTART -u UNIONSIZE -e "
          "ESTFILENAME [-r ESTINFLATIONRATIO -i INPUTSETFILENAME -p "
          "PBSBCHPARAMFILENAME -R MAXROUNDS -g AVGDIFFSINEACHGROUP]\n",
          progname);
  exit(1);
}

// a wrapper for xxhash
uint myhash(uint key, uint seed) { return XXH32(&key, sizeof(key), seed); }

// parity encoding v2
void Encode(const vector<uint> &set,  // elements in a set
            vector<uint> &bit,        // parity bits after encoding
            vector<uint> &loc,        // hash value of each element
            vector<uint> &elm_xor,    // xor of each bin
            uint seed                 // hash seed
) {
  size_t len = set.size();
  size_t n = bit.size();
  std::fill(bit.begin(), bit.end(), 0);
  std::fill(elm_xor.begin(), elm_xor.end(), 0);

  for (size_t i = 0; i < len; ++i) {
    // make sure no one goes into the bin with id 0
    // since minisketch does not support it right now
    loc[i] = (myhash(set[i], seed) % (n - 1)) + 1;
    bit[loc[i]] ^= 1u;
    elm_xor[loc[i]] ^= set[i];
  }
}

struct ExperimentParams {
#define DEFAULT_INPUT_SETS_FILENAME \
  "../../test-sets/sets_100000_1000_32_1574465740.txt"
#define SIGNATURE_WIDTH 32
  size_t num_tests;
  size_t union_size;
  size_t diff_size;
  size_t diff_start;

  std::string diff_est_filename;
  std::string input_sets_filename;

  ExperimentParams()
      : num_tests(0),
        union_size(0),
        diff_size(0),
        diff_start(union_size),
        diff_est_filename(""),
        input_sets_filename(DEFAULT_INPUT_SETS_FILENAME) {}

  void print(FILE *stream = stdout) const {
    fprintf(stream,
            "num_tests                    = %lu\n"
            "union_size                   = %lu\n"
            "diff_size                    = %lu\n"
            "diff_start                   = %lu\n"
            "signature_width              = %u\n"
            "diff_est_filename            = %s\n"
            "input_sets_filename          = %s\n",
            num_tests, union_size, diff_size, diff_start, SIGNATURE_WIDTH,
            diff_est_filename.c_str(), input_sets_filename.c_str());
  }

  bool validate() const {
    return (num_tests > 0 && union_size > 0 && diff_size > 0 &&
            diff_size <= union_size && diff_start >= 0 &&
            diff_start < union_size);
  }

  std::vector<float> loadEstDiffSizes() const {
    std::ifstream diff_estimations_fp;
    std::vector<float> est_diff_sizes;
    if (!diff_est_filename.empty()) {
      diff_estimations_fp.open(diff_est_filename);
      if (!diff_estimations_fp.is_open()) {
        fprintf(stderr, "Failed to open %s\n", diff_est_filename.c_str());
        return {};
      }
      for (size_t i = 0; i < num_tests && !diff_estimations_fp.eof(); ++i) {
        est_diff_sizes.push_back(0);
        diff_estimations_fp >> est_diff_sizes.back();
      }
      assert(est_diff_sizes.size() == num_tests);
      return est_diff_sizes;
    }
    return {};
  }
};

struct PbsParams {
#define DEFAULT_MAX_ROUNDS 3
#define DEFAULT_AVG_DIFFS_PER_GROUP 5
#define DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL 3
#define DEFAULT_CHECKSUM_LENGTH 32
#define DEFAULT_EST_INFLATION_RATIO 1.0
#define DEFAULT_PARAM_FILENAME_PREFIX "../perf/params_r"
  /** user-configurable parameters */
  unsigned max_rounds;
  float avg_diffs_per_group;
  unsigned num_groups_when_bch_fail;
  unsigned checksum_len;
  /** auto-optimized parameters */
  unordered_map<unsigned, pair<unsigned /* bch length */,
                               unsigned /* bch decoding capacity */>>
      best_bch_params;

  /** externally-dependent parameters (default: 1.0) */
  float est_inflation_ratio;
  std::string best_bch_params_filename;
  vector<uint> seeds;

  PbsParams()
      : max_rounds(DEFAULT_MAX_ROUNDS),
        avg_diffs_per_group(DEFAULT_AVG_DIFFS_PER_GROUP),
        num_groups_when_bch_fail(DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL),
        checksum_len(DEFAULT_CHECKSUM_LENGTH),
        best_bch_params{},
        est_inflation_ratio(DEFAULT_EST_INFLATION_RATIO),
        best_bch_params_filename(std::string(DEFAULT_PARAM_FILENAME_PREFIX) +
                                 std::to_string(max_rounds) + ".csv") {}

  bool loadBestParams() {
    fprintf(stdout, "Loading best BCH parameters for PBS from \"%s\" ... ",
            best_bch_params_filename.c_str());
    string line;
    ifstream fp(best_bch_params_filename);
    if (!fp.is_open()) {
      std::cerr << "Unable to open parameter file: " << best_bch_params_filename
                << "\n";
      return false;
    }
    std::string results[4];

    while (getline(fp, line)) {
      std::istringstream iss(line);
      int j = 0;
      while (std::getline(iss, results[j], ',')) {
        j++;
      }
      unsigned expected_diff_size = std::stoul(results[0]);
      unsigned bch_length = std::stoul(results[2]);
      unsigned bch_capacity = std::stoul(results[3]);
      assert(best_bch_params.count(expected_diff_size) == 0);
      best_bch_params[expected_diff_size] = {bch_length, bch_capacity};
    }
    fp.close();
    fprintf(stdout, "DONE!\n");
    return true;
  }

  void print(FILE *stream = stdout) const {
    fprintf(stream,
            "max_rounds                   = %u\n"
            "avg_diffs_per_group          = %.2f\n"
            "num_groups_when_bch_fail     = %u\n"
            "checksum_len                 = %u\n"
            "est_inflation_ratio          = %.2f\n"
            "best_bch_params_filename     = %s\n",
            max_rounds, avg_diffs_per_group, num_groups_when_bch_fail,
            checksum_len, est_inflation_ratio,
            best_bch_params_filename.c_str());
  }

  bool validate() const {
    return ((max_rounds > 0 && avg_diffs_per_group > 0 &&
             num_groups_when_bch_fail > 0 && (!best_bch_params.empty())));
  }

  void loadSeed(size_t num_tests) {
    fprintf(stdout, "Loading all random seeds ... ");
#ifdef USE_PY_SEEDS
    seeds = pbsutil::loadSeedPool();
    assert(seeds.size() >= num_tests);
    seeds.resize(num_tests);
#else
    seeds = pbsutil::GenerateRandom32u(num_tests);
#endif

    fprintf(stdout, "DONE!\n");
  }

  void prepareExperiments(const ExperimentParams &experiment_params) {
    loadSeed(experiment_params.num_tests);
    loadBestParams();
  }
};

void arg_parse(int argc, char *argv[], ExperimentParams &experiment_params,
               PbsParams &pbs_params) {
  int c;
  char *progname;
  char *p;
  progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

  while (true) {
    static struct option long_options[] = {
        {"number-test", required_argument, nullptr, 'n'},
        {"diff-size", required_argument, nullptr, 'd'},
        {"diff-start", required_argument, nullptr, 's'},
        {"union-size", required_argument, nullptr, 'u'},
        {"est-filename", required_argument, nullptr, 'e'},
        {"input-sets-filename", optional_argument, nullptr, 'i'},
        {"bch-param-filename", optional_argument, nullptr, 'p'},
        {"est-inflation-ratio", optional_argument, nullptr, 'r'},
        {"max-rounds", optional_argument, nullptr, 'R'},
        {"group-size", optional_argument, nullptr, 'g'},
        {nullptr, 0, nullptr, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "n:d:s:u:e:r:i:p:R:g:h", long_options,
                    &option_index);

    /* Detect the end of the options. */
    if (c == -1) {
      break;
    }

    switch (c) {
      case 'n':
        debug("option -n with value `%s'\n", optarg);
        experiment_params.num_tests = stoul(optarg);
        break;
      case 'd':
        debug("option -d with value `%s'\n", optarg);
        experiment_params.diff_size = stoul(optarg);
        break;
      case 's':
        debug("option -s with value `%s'\n", optarg);
        experiment_params.diff_start = stoul(optarg);
        break;
      case 'u':
        debug("option -u with value `%s'\n", optarg);
        experiment_params.union_size = stoul(optarg);
        break;
      case 'e':
        debug("option -e with value `%s'\n", optarg);
        experiment_params.diff_est_filename = optarg;
        break;
      case 'p':
        debug("option -p with value `%s'\n", optarg);
        pbs_params.best_bch_params_filename = optarg;
        break;
      case 'i':
        debug("option -i with value `%s'\n", optarg);
        experiment_params.input_sets_filename = optarg;
        break;
      case 'r':
        debug("option -r with value `%s'\n", optarg);
        pbs_params.est_inflation_ratio = stof(optarg);
        break;
      case 'R':
        debug("option -R with value `%s'\n", optarg);
        pbs_params.max_rounds = stoul(optarg);
        break;
      case 'g':
        debug("option -g with value `%s'\n", optarg);
        pbs_params.avg_diffs_per_group = stof(optarg);
        break;
      default:
        usage(progname);
    }
  }

  experiment_params.print();
  pbs_params.print();
  pbs_params.prepareExperiments(experiment_params);

  if (!(experiment_params.validate() && pbs_params.validate())) {
    fprintf(stderr, "Invalid parameters!");
    exit(1);
  }
}

struct Metric {
  // communication
  vector<std::size_t> com_bch_encoding;  // sketch sizes
  vector<std::size_t> com_bch_decoding;  // header plus bin indices
  vector<std::size_t>
      com_bch_decoding_verify;       // header plus "index" for exception I & II
  vector<std::size_t> com_xor;       // xor of decoded bins
  vector<std::size_t> com_checksum;  // checksum

  // computation
  vector<double> t_decoding;
  vector<double> t_encoding;

  // general
  vector<std::size_t> g_bch_decoding_failure;
  vector<std::size_t> g_num_groups;
  vector<std::size_t> g_exception_I;
  vector<std::size_t> g_exception_II;
  vector<std::size_t> g_true_recon;
  vector<std::size_t> g_false_recon;

  void clear() {
    com_bch_encoding.clear();
    com_bch_decoding.clear();
    com_bch_decoding_verify.clear();
    com_xor.clear();
    com_checksum.clear();

    t_decoding.clear();
    t_encoding.clear();

    g_bch_decoding_failure.clear();
    g_num_groups.clear();
    g_exception_I.clear();
    g_exception_II.clear();
    g_true_recon.clear();
    g_false_recon.clear();
  }

  double total_bytes(size_t i) const {
    auto total_bits =
        (double)(com_bch_decoding[i] + com_bch_encoding[i] +
                 com_bch_decoding_verify[i] + com_xor[i] + com_checksum[i]);
    return total_bits / 8.0;
  }

  static const char *header() {
    return "#tid,round,transmitted_bytes,succeed,encoding_time,decoding_time,"
           "truerec,falserec,paritybits,"
           "xornum,checksumbits,decodebits,checkbits,logn,t,assumed_diff_size,"
           "bch_failure,num_groups,failure_exception_I,failure_execption_II";
  }

  static const char *fmt() {
    return "%u,%u,%.2f,%d,%.8f,%.8f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%"
           "u\n";
  }
};

int main(int argc, char **argv) {
  ExperimentParams experiment_params;
  PbsParams pbs_params;
  Metric metric;
  fprintf(stdout, "Parsing arugments ... \n");
  arg_parse(argc, argv, experiment_params, pbs_params);
  fprintf(stdout, "DONE!\n\n");

  FILE *ofp = nullptr;

  auto output_filename =
      to_string(experiment_params.union_size) + "_" +
      to_string(experiment_params.diff_size) + "_" +
      to_string(pbs_params.avg_diffs_per_group) + "_" +
      (experiment_params.diff_est_filename.empty() ? "0" : "1") + "_r" +
      to_string(pbs_params.max_rounds) + "_communication_PBS.txt";

  ofp = fopen(output_filename.c_str(), "w");

  if (ofp == nullptr) {
    fprintf(stderr, "Failed to open file %s!\n", output_filename.c_str());
    exit(1);
  }

  experiment_params.print(ofp);
  pbs_params.print(ofp);

  fprintf(ofp, "%s\n", Metric::header());

  //
  TxTFileReader reader(experiment_params.input_sets_filename);
  // skip 3 lines (since they are meta data for those sets)
  reader.skip_lines(3);
  // sets A & B
  vector<uint> setA, setB;
  vector<vector<uint>> xorA, xorB;
  // ground truth
  unordered_set<uint> diff;
  // sizes (cardinalities) of two sets
  size_t size_a, size_b;
  size_t initial_num_groups;
  size_t lg_j = 0, lg_k = 0, lg_m = 0;
  size_t scaled_diff_size = 0;
  double t_partition = 0.0;
  setB.resize(experiment_params.union_size - experiment_params.diff_size);
  auto est_diff_sizes = experiment_params.loadEstDiffSizes();

  SimpleTimer timer;
  // start testing
  for (size_t i = 0; i < experiment_params.num_tests; ++i) {
    // read set A
    reader.read<uint>(setA, experiment_params.union_size);
    for (lg_j = 0, lg_k = 0; lg_j < experiment_params.diff_start;
         ++lg_j, ++lg_k) {
      setB[lg_k] = setA[lg_j];
    }
    diff.clear();
    for (lg_m = 0; lg_m < experiment_params.diff_size; ++lg_m, ++lg_j) {
      diff.insert(setA[lg_j]);
    }
    for (; lg_j < experiment_params.union_size; ++lg_j, ++lg_k) {
      setB[lg_k] = setA[lg_j];
    }

    assert((size_t)lg_k == setB.size());

    size_a = setA.size();
    size_b = setB.size();

    if (!est_diff_sizes.empty()) {
      // In the case where the ground truth value of the size
      // of the set difference is not known.
      // our set reconciliation will assume the size of the set difference
      // is `scaled_diff_size`
      // NOTE: we first convert the estimated diff in double, because we have
      // encountered precision issue (which resulted in different results when
      // using float and double). example: 37726.8125 using float:
      // scaled_diff_size = 52063 using double: scaled_diff_size = 52064
      scaled_diff_size = size_t(
          ceil((double)est_diff_sizes[i] * pbs_params.est_inflation_ratio));
    }

    // partition
    timer.restart();
    // number of groups that are to be reconciled
    size_t remaining_num_groups =
        ceil((float)scaled_diff_size / pbs_params.avg_diffs_per_group);
    initial_num_groups = remaining_num_groups;
    vector<vector<uint>> subA(remaining_num_groups);
    vector<vector<uint>> subB(remaining_num_groups);
    vector<size_t> group_id(remaining_num_groups);
    vector<uint> reconciled;
    // partitioning
    for (size_t j = 0; j < size_a; ++j) {
      size_t index =
          myhash(setA[j], pbs_params.seeds[i]) % remaining_num_groups;
      subA[index].push_back(setA[j]);
    }
    for (size_t j = 0; j < size_b; ++j) {
      size_t index =
          myhash(setB[j], pbs_params.seeds[i]) % remaining_num_groups;
      subB[index].push_back(setB[j]);
    }
    // recording group id
    for (size_t j = 0; j < remaining_num_groups; ++j) {
      group_id[j] = j;
    }
    t_partition += timer.elapsed();

    // load params
    if (pbs_params.best_bch_params.count(scaled_diff_size) == 0) {
      fprintf(stderr,
              "Something was wrong with the inputs.\nDiffSize = %.4f, "
              "ScaledDiffSize = %lu (SHOULD BE %lu), bestParamSize: %lu\n",
              est_diff_sizes[i], scaled_diff_size,
              size_t(ceil(est_diff_sizes[i] * pbs_params.est_inflation_ratio)),
              pbs_params.best_bch_params.size());
      exit(1);
    }
    auto param_pair = pbs_params.best_bch_params[scaled_diff_size];
    size_t logn = param_pair.first;
    size_t t = param_pair.second;
    size_t n = (1u << logn) - 1u;
    const size_t decode_header_len = ceil(log2(t + 2));
    const std::size_t sketch_each_len = logn * t;

    /** Message Header Format in PBS
     *  0                                ceil(log2(t + 2))
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  +  message header for each group  +
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     *  As shown above, for each group, we use $\lceil\log_2(t+2)\rceil$ bits
     * longer header to tell how many elements were decoded with BCH, where t is
     * the BCH decoding capacity. More precisely, a header value of $x \in
     * \{0,1,2,...,t\} means, there are x elements decoded, whereas a value of
     * $t+1$ (or -1) means decoding failed. Therefore, we need at most
     * $\lceil\log_2(t+2)\rceil$ bits for each group.
     *
     *  Followed by this header are the data field, which contains the indices
     * of the corresponding x (assuming the header value is $x \in
     * \{0,1,2,...,t\}) bins in this group, each of which takes $\log_2 n$ bits.
     * Then, the x+1 XORs, each of the first x XORs is the bitwise XOR for all
     * elements located in the corresponding bin, and the last one is a checksum
     * for the entire group.
     *
     *  Note that if BCH decoding failed, then a packet with only the header
     * message will be sent.
     *
     *  Since PBS allows multiple rounds, the host, upond receiving this packet,
     * should verify that result. If it finds that there are some remaining
     * un-reconciled elements, it should send a message to tell the other host.
     * In this implementation, we assume the following message format is used.
     *
     *  The 1st bit tells whether exception I (even number of distinct elements
     * partitining into the same bin) was detected, since in such a case, we
     * DONOT know which bin is. So, we should reconcile all elements in the bins
     * which do not belong to the x decoded bins. Then, we should use \lceil
     * \log_2 (x + 1) \rceil bits to tell whether exception II happens in the x
     * decoded bins.
     *
     * */

    int num_running_rounds = 1;

    metric.clear();

    while (remaining_num_groups > 0) {
      // initialize metric's member variables
      metric.com_bch_encoding.push_back(remaining_num_groups * sketch_each_len);
      metric.com_checksum.push_back(remaining_num_groups *
                                    pbs_params.checksum_len);
      metric.com_bch_decoding.push_back(remaining_num_groups *
                                        decode_header_len);
      metric.com_xor.push_back(0);
      metric.com_bch_decoding_verify.push_back(0);

      metric.t_decoding.push_back(0.0);
      metric.t_encoding.push_back(t_partition);

      metric.g_bch_decoding_failure.push_back(0);
      metric.g_exception_I.push_back(0);
      metric.g_exception_II.push_back(0);
      metric.g_num_groups.push_back(remaining_num_groups);
      metric.g_true_recon.push_back(0);
      metric.g_false_recon.push_back(0);

      vector<size_t> failed_subsets;
      xorA.resize(remaining_num_groups);
      xorB.resize(remaining_num_groups);

      double dummy_patition_time = 0.0;  // for addressing exception I & II

      for (size_t j = 0; j < remaining_num_groups; ++j) {
        timer.restart();
        xorA[j].resize(n);
        xorB[j].resize(n);
        size_t subset_a_size = subA[j].size();
        size_t subset_b_size = subB[j].size();
        vector<uint> bitA(n);
        vector<uint> bitB(n);
        vector<uint> locA(subset_a_size);
        vector<uint> locB(subset_b_size);
        // partition into groups
        Encode(subA[j], bitA, locA, xorA[j],
               pbs_params.seeds[i] + num_running_rounds);
        Encode(subB[j], bitB, locB, xorB[j],
               pbs_params.seeds[i] + num_running_rounds);
        metric.t_encoding.back() += timer.elapsed();

        // BCH encoding
        timer.restart();
        minisketch *sketch_a = minisketch_create(logn, 0, t);
        minisketch *sketch_b = minisketch_create(logn, 0, t);
        for (size_t k = 0; k < n; ++k) {
          if (bitA[k]) minisketch_add_uint64(sketch_a, k);
        }
        for (size_t k = 0; k < n; ++k) {
          if (bitB[k]) minisketch_add_uint64(sketch_b, k);
        }
        metric.t_encoding.back() += timer.elapsed();

        // BCH decoding
        timer.restart();
        vector<uint64_t> pos(t);
        minisketch_merge(sketch_b, sketch_a);
        int p = minisketch_decode(sketch_b, t, &pos[0]);
        minisketch_destroy(sketch_a);
        minisketch_destroy(sketch_b);
        metric.t_decoding.back() += timer.elapsed();

        if (p >= 0) {  // decoding succeeded
          {
            // do measurement
            metric.com_xor.back() += p * SIGNATURE_WIDTH;
            metric.com_bch_decoding.back() += p * logn;
            // one bit for indicating exception I, the rest for indicating
            // whether there is exception II
            metric.com_bch_decoding_verify.back() += ceil(log2(p + 1)) + 1;
          }

          // calculate XORs
          timer.restart();
          // ???: changed from (t+1) => (p+1)
          vector<uint> XOR(p + 1, 0);
          vector<uint> ind(n, 0);

          for (int k = 0; k < p; ++k) {
            ind[pos[k]] = k + 1;
          }
          for (size_t bi = 1; bi < n; ++bi) {
            XOR[ind[bi]] ^= (xorA[j][bi] ^ xorB[j][bi]);
          }
          metric.t_decoding.back() += timer.elapsed();

          std::vector<int> exception_II_groups;

          // xor decoding
          timer.restart();
          if (XOR[0] != 0) metric.g_exception_I.back()++;
          for (int k = 1; k <= p; ++k) {
            if (XOR[k]) {// odd number of distinct elements
              if (myhash(XOR[k], pbs_params.seeds[i] + num_running_rounds) %
                              (n - 1) +
                          1 ==
                      pos[k - 1] &&
                  myhash(XOR[k], pbs_params.seeds[i]) % initial_num_groups ==
                      (unsigned)group_id[j]) {
                reconciled.push_back(XOR[k]);
                if (diff.count(XOR[k]))
                  metric.g_true_recon.back()++;
                else
                  metric.g_false_recon.back()++;
              } else {
                metric.g_exception_II.back()++;
                exception_II_groups.push_back(k);
              }
            }
          }
          metric.t_decoding.back() += timer.elapsed();  // time to decode xor

          timer.restart();
          // The XORs of all the elements in the buckets whose
          // does not contain any balls should be zero!
          // If it is not ===> collision happens (even number of balls go into
          // the same bin)
          if (XOR[0] != 0) {
            size_t old_size = subA.size();
            subA.resize(old_size + 1);
            subB.resize(old_size + 1);
            group_id.resize(old_size + 1);
            for (size_t k = 0; k < subset_a_size; ++k) {
              if (!ind[locA[k]]) {
                subA[old_size].push_back(subA[j][k]);
              }
            }
            for (size_t k = 0; k < subset_b_size; ++k) {
              if (!ind[locB[k]]) {
                subB[old_size].push_back(subB[j][k]);
              }
            }
            group_id[old_size] = group_id[j];
          }

          // for (int k = 1; k <= p; ++k) {
          //   if (XOR[k]) {
          //     if (myhash(XOR[k], pbs_params.seeds[i] + num_running_rounds) %
          //                     (n - 1) +
          //                 1 ==
          //             pos[k - 1] &&
          //         myhash(XOR[k], pbs_params.seeds[i]) % initial_num_groups ==
          //             (unsigned)group_id[j]) {
          //       reconciled.push_back(XOR[k]);
          //       if (diff.count(XOR[k]))
          //         metric.g_true_recon.back()++;
          //       else
          //         metric.g_false_recon.back()++;
          //     } else {
          //       metric.g_exception_II.back()++;
          //       // here, we can actually save some space, for example, we
          //       // can use use log_2 p bits. However, this event happens
          //       // rarely, this saving is negligible.
          //       metric.com_bch_decoding_verify.back() += logn;
          //       size_t old_size = subA.size();
          //       subA.resize(old_size + 1);
          //       subB.resize(old_size + 1);
          //       group_id.resize(old_size + 1);
          //       for (size_t q = 0; q < subset_a_size; ++q) {
          //         if (locA[q] == pos[k - 1]) {
          //           subA[old_size].push_back(subA[j][q]);
          //         }
          //       }
          //       for (size_t q = 0; q < subset_b_size; ++q) {
          //         if (locB[q] == pos[k - 1]) {
          //           subB[old_size].push_back(subB[j][q]);
          //         }
          //       }
          //       group_id[old_size] = group_id[j];
          //     }
          //   }
          // }
          for (int k : exception_II_groups) {
            // if (XOR[k]) {
            //   if (myhash(XOR[k], pbs_params.seeds[i] + num_running_rounds) %
            //                   (n - 1) +
            //               1 ==
            //           pos[k - 1] &&
            //       myhash(XOR[k], pbs_params.seeds[i]) % initial_num_groups ==
            //           (unsigned)group_id[j]) {
            //     reconciled.push_back(XOR[k]);
            //     if (diff.count(XOR[k]))
            //       metric.g_true_recon.back()++;
            //     else
            //       metric.g_false_recon.back()++;
            //   } else {
            //     metric.g_exception_II.back()++;
            // here, we can actually save some space, for example, we
            // can use use log_2 p bits. However, this event happens
            // rarely, this saving is negligible.
            metric.com_bch_decoding_verify.back() += logn;
            size_t old_size = subA.size();
            subA.resize(old_size + 1);
            subB.resize(old_size + 1);
            group_id.resize(old_size + 1);
            for (size_t q = 0; q < subset_a_size; ++q) {
              if (locA[q] == pos[k - 1]) {
                subA[old_size].push_back(subA[j][q]);
              }
            }
            for (size_t q = 0; q < subset_b_size; ++q) {
              if (locB[q] == pos[k - 1]) {
                subB[old_size].push_back(subB[j][q]);
              }
            }
            group_id[old_size] = group_id[j];
          }
          //   }
          // }
          dummy_patition_time += timer.elapsed();  // time to decode xor

        } else {  // p = -1, failed to decode
          failed_subsets.push_back(j);
          metric.g_bch_decoding_failure.back()++;
        }
      }

      t_partition = dummy_patition_time;  //

      // address bch decoding failure exception
      if (!failed_subsets.empty()) {
        timer.restart();
        for (auto j : failed_subsets) {
          size_t old_size = subA.size();
          size_t subset_a_size = subA[j].size();
          size_t subset_b_size = subB[j].size();
          subA.resize(old_size + pbs_params.num_groups_when_bch_fail);
          subB.resize(old_size + pbs_params.num_groups_when_bch_fail);
          group_id.resize(old_size + pbs_params.num_groups_when_bch_fail);
          for (size_t k = 0; k < subset_a_size; ++k) {
            size_t index =
                myhash(subA[j][k], pbs_params.seeds[i] + num_running_rounds) %
                pbs_params.num_groups_when_bch_fail;
            subA[old_size + index].push_back(subA[j][k]);
          }
          for (size_t k = 0; k < subset_b_size; ++k) {
            size_t index =
                myhash(subB[j][k], pbs_params.seeds[i] + num_running_rounds) %
                pbs_params.num_groups_when_bch_fail;
            subB[old_size + index].push_back(subB[j][k]);
          }
          for (size_t k = 0; k < pbs_params.num_groups_when_bch_fail; ++k) {
            group_id[old_size + k] = group_id[j];
          }
        }
        t_partition += timer.elapsed();
      }
      // update number of groups
      subA.erase(subA.begin(), subA.begin() + remaining_num_groups);
      subB.erase(subB.begin(), subB.begin() + remaining_num_groups);
      group_id.erase(group_id.begin(), group_id.begin() + remaining_num_groups);
      remaining_num_groups = subA.size();

      //
      xorA.clear();
      xorB.clear();
      ++num_running_rounds;
    }

    unsigned recovered = 0;
    for (int j = 0; j < num_running_rounds - 1; ++j) {
      recovered += metric.g_true_recon[j];
      fprintf(ofp, Metric::fmt(), i + 1, j + 1, metric.total_bytes(j),
              (recovered == experiment_params.diff_size ? 1 : 0),
              metric.t_encoding[j], metric.t_decoding[j],
              metric.g_true_recon[j], metric.g_false_recon[j],
              metric.com_bch_encoding[j], metric.com_xor[j],
              metric.com_checksum[j], metric.com_bch_decoding[j],
              metric.com_bch_decoding_verify[j], logn, t, scaled_diff_size,
              metric.g_bch_decoding_failure[j], metric.g_num_groups[j],
              metric.g_exception_I[j], metric.g_exception_II[j]);
    }
  }
  fclose(ofp);
  return 0;
}