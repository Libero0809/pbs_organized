#!/usr/bin/env python3
import numpy as np
import json
import os.path

STARTER_FILE = "../test-sets/diff_starter.json"
ESTIMATE_FILE_PAT = "../test-sets/diff_estimates_with_tow_{d}_128_9012.txt"
INFLATION_RATIO = 1.38 # for 128
UNION_SIZE = 100000
N_TEST = 1000
AVG = 5
FURTHER_DECOMP = 3

def diff_start_gen():
    if os.path.exists(STARTER_FILE):
        with open(STARTER_FILE, 'r') as fp:
            return json.load(fp)
    diffs = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000 16000 30000 50000 100000"
    diff_sizes = [int(s) for s in diffs.split()]
    starter = [0] + list(np.cumsum(diff_sizes)[:-1])
    starter = [int(x) for x in starter]

    j_obj = dict(zip([str(d) for d in diff_sizes], starter))
    with open(STARTER_FILE, 'w') as fp:
        json.dump(j_obj, fp, indent=4)
    return j_obj


def no_est(diff_sizes):
    starter = diff_start_gen()
    for d in [int(s) for s in diff_sizes.split()]:
        print(f'echo "./Erlay -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE}"')
        print(f"./Erlay -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE}")


def est99(diff_sizes):
    starter = diff_start_gen()
    for d in [int(s) for s in diff_sizes.split()]:
        print(f'echo "./Erlay -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}"')
        print(f"./Erlay -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}")


def no_est_wp(diff_sizes):
    starter = diff_start_gen()
    for r in [2,3]:
        for d in [int(s) for s in diff_sizes.split()]:
            print(f'echo "./ErlayWP -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r}"')
            print(f"./ErlayWP -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r}")


def est_wp_match(diff_sizes):
    starter = diff_start_gen()
    for r in [2,3]:
        for d in [int(s) for s in diff_sizes.split()]:
            print(f'echo "./ErlayWPMatch -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}"')
            print(f"./ErlayWPMatch -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}")


def est_wp_tuning(diff_sizes):
    starter = diff_start_gen()
    for r in [2,3]:
        for d in [int(s) for s in diff_sizes.split()]:
            print(f'echo "./ErlayWPTuning -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}"')
            print(f"./ErlayWPTuning -n {N_TEST} -d {d} -s {starter[str(d)]} -u {UNION_SIZE} -m {AVG} - c {FURTHER_DECOMP} -R {r} -e {ESTIMATE_FILE_PAT.format(d=d)} -r {INFLATION_RATIO}")


if __name__ == '__main__':
    #no_est("10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000")
    # est99("10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000")
    # no_est_wp("10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000")
    # est_wp_match("10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000")
    est_wp_tuning("10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000")