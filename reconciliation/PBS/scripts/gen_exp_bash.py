#!/usr/bin/env python3
import numpy as np
import json
import os.path

STARTER_FILE = "../diff_starter.json"
N_TEST = 1000
UNION_SIZE = 100000
AD = 5.0
BINARY = 'ITRecon'
ESTIMATE_FILE_PAT = "../test-sets/diff_estimates_with_tow_{d}_128_9012.txt"
INFLATION_RATIO = 1.38 # for 128


def diff_start_gen():
    if os.path.exists(STARTER_FILE):
        with open(STARTER_FILE, "r") as fp:
            return json.load(fp)
    diffs = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000 16000 30000 50000 100000"
    diff_sizes = [int(s) for s in diffs.split()]
    starter = [0] + list(np.cumsum(diff_sizes)[:-1])
    starter = [int(x) for x in starter]

    j_obj = dict(zip([str(d) for d in diff_sizes], starter))
    with open(STARTER_FILE, "w") as fp:
        json.dump(j_obj, fp, indent=4)
    return j_obj


def varying_decoding_capacity(diffs, cap_inflation_ratios):
    logn = 10
    starter = diff_start_gen()
    for r in cap_inflation_ratios:
        t = int(np.ceil(r * AD))
        for d in [int(s) for s in diffs.split()]:
            print(
                f'echo "./set_recon {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]}"'
            )
            print(
                f"./set_recon {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]}"
            )


def best_parameters_known_d(diffs, parameter_filename):
    starter = diff_start_gen()
    import json
    binary = "ITRecon"

    with open(parameter_filename, "r") as fp:
        parameters = json.load(fp)
        for r in [2, 3]:
            best_param = parameters[f"round={r}"]
            for d in [int(s) for s in diffs.split()]:
                d_str = str(d)
                logn = best_param["bch_length"][d_str]
                t = best_param["bch_capacity"][d_str]
                print(
                    f'echo "./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r}"'
                )
                print(
                    f"./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r}"
                )


def best_parameters(diffs, parameter_filename):
    starter = diff_start_gen()
    import json

    with open(parameter_filename, "r") as fp:
        parameters = json.load(fp)
        for r in [2, 3]:
            best_param = parameters[f"round={r}"]
            for d in [int(s) for s in diffs.split()]:
                d_str = str(d)
                logn = best_param["bch_length"][d_str]
                t = best_param["bch_capacity"][d_str]
                print(
                    f'echo "./set_recon {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]}"'
                )
                print(
                    f"./set_recon {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]}"
                )

def best_parameters_v2(diffs, parameter_filename):
    starter = diff_start_gen()
    import json
    binary = "ITRecon"
    with open(parameter_filename, "r") as fp:
        parameters = json.load(fp)
        for r in [2, 3]:
            best_param = parameters[f"round={r}"]
            for d in [int(s) for s in diffs.split()]:
                d_str = str(d)
                logn = best_param["bch_length"][d_str]
                t = best_param["bch_capacity"][d_str]
                print(
                    f'echo "./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r} {ESTIMATE_FILE_PAT.format(d=d)} {INFLATION_RATIO}"'
                )
                print(
                    f"./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r} {ESTIMATE_FILE_PAT.format(d=d)} {INFLATION_RATIO}"
                )


def best_parameters_vs_graphene(diffs, parameter_filename):
    starter = diff_start_gen()
    import json
    binary = "ITReconAgainstGraphene"
    with open(parameter_filename, "r") as fp:
        parameters = json.load(fp)
        for r in [2, 3]:
            best_param = parameters[f"round={r}"]
            for d in [int(s) for s in diffs.split()]:
                d_str = str(d)
                logn = best_param["bch_length"][d_str]
                t = best_param["bch_capacity"][d_str]
                print(
                    f'echo "./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r} {ESTIMATE_FILE_PAT.format(d=d)} {INFLATION_RATIO}"'
                )
                print(
                    f"./{binary} {N_TEST} {UNION_SIZE} {d} {1.0 / AD:.6f} {logn} {t} 3 {starter[str(d)]} {r} {ESTIMATE_FILE_PAT.format(d=d)} {INFLATION_RATIO}"
                )


if __name__ == "__main__":
    # varying_decoding_capacity(
    #     "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000",
    #     np.arange(1.5, 4.5, 0.5),
    # )
    # best_parameters(
    #     "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000",
    #     "parameters.json"
    # )
    # best_parameters_known_d(
    #     "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000",
    #     "parameters.json"
    # )
    # best_parameters_v2(
    #     "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000",
    #     "parameters.json"        
    # )
    best_parameters_vs_graphene(
        "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000",
        "parameters.json"        
    )
    # capacities = [7, 8, 9, 10]
    # avg_d = [3, 4, 5, 6, 7, 8]
    # t_ratio = [1.5, 2.0, 2.5, 3, 3.5]

    # set_diff = [10, 100, 1000, 10000]
    # starter = diff_start_gen()

    # for c in capacities:
    #     for ad in avg_d:
    #         for tr in t_ratio:
    #             t = int(np.ceil(ad * tr))
    #             for i, d in enumerate(set_diff):
    #                 print(f'echo "./set_recon 100 100000 {d} {1.0 / ad:.6f} {c} {t} 3 {starter[str(d)]}"')
    #                 print(f'./set_recon 100 100000 {d} {1.0 / ad:.6f} {c} {t} 3 {starter[str(d)]}')
