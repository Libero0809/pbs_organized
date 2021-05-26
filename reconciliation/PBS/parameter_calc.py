#!/usr/bin/env python3
import numpy as np
from math import ceil
import os

balls_into_bins_dict = {}
transition_dict = {}
round_table_dict = {}


def calc_balls_into_bins(n_bins, n_balls):
    # result[i, j, k] is the probability of putting i balls into n_bins bins causing j bins are empty and k bins have exactly 1 ball
    result = np.zeros((n_balls + 1, n_bins + 1, n_balls + 2))
    result[0, n_bins, 0] = 1
    result[1, n_bins - 1, 1] = 1
    for i in range(2, n_balls + 1):
        for j in range(n_bins - i, n_bins):
            for k in range(0, n_bins + 1 - j):
                result[i, j, k] = (
                    result[i - 1, j + 1, k - 1] * (j + 1) / n_bins
                    + result[i - 1, j, k + 1] * (k + 1) / n_bins
                    + result[i - 1, j, k] * (n_bins - j - k) / n_bins
                )
    return result


def get_balls_into_bins(n_bins, n_balls):
    if (n_bins, n_balls) not in balls_into_bins_dict:
        balls_into_bins_dict[(n_bins, n_balls)] = calc_balls_into_bins(n_bins, n_balls)
    return balls_into_bins_dict[(n_bins, n_balls)]


def calc_transition(balls_into_bins, max_decode_size):
    n_balls = balls_into_bins.shape[0] - 1
    n_bins = balls_into_bins.shape[1] - 1
    result = np.zeros((n_balls + 1, n_balls + 1))
    for i in range(0, n_balls + 1):
        for j in range(0, i + 1):
            result[i, j] = np.sum(
                balls_into_bins[i, n_bins - max_decode_size : n_bins + 1, i - j]
            )
    for i in range(max_decode_size + 1, n_balls + 1):
        result[i, i] = 1 - np.sum(result[i, 0:i])
    return result


def get_transition(n_bins, n_balls, max_decode_size):
    balls_into_bins = get_balls_into_bins(n_bins, n_balls)
    n_bins = balls_into_bins.shape[1] - 1
    n_balls = balls_into_bins.shape[0] - 1
    if (n_bins, n_balls, max_decode_size) not in transition_dict:
        transition_dict[(n_bins, n_balls, max_decode_size)] = calc_transition(
            balls_into_bins, max_decode_size
        )
    return transition_dict[(n_bins, n_balls, max_decode_size)]


def calc_round(n_bins, n_balls, max_decode_size, max_round):
    transition = get_transition(n_bins, n_balls, max_decode_size)
    result = np.zeros((n_balls + 1, max_round + 1))
    iter = transition
    for r in range(1, max_round + 1):
        result[:, r] = 1 - iter[:, 0]
        iter = np.matmul(iter, transition)
    result[:, 0] = 1
    return result


def get_round(n_bins, n_balls, max_decode_size, max_round):
    if (n_bins, n_balls, max_decode_size, max_round) not in round_table_dict:
        round_table_dict[(n_bins, n_balls, max_decode_size, max_round)] = calc_round(
            n_bins, n_balls, max_decode_size, max_round
        )
    return round_table_dict[(n_bins, n_balls, max_decode_size, max_round)]


def calc_prob_fail_upperbound_no_split(
    total_balls, n_groups, n_bins, max_round, split_num, max_decode_size, round_table
):
    prob = (1 - 1 / n_groups) ** total_balls
    prob_fail = 0
    prob_tail = 1 - prob
    for n_balls in range(1, max_decode_size):
        prob = prob * (total_balls - n_balls + 1) / n_balls / (n_groups - 1)
        prob_fail += prob * round_table[n_balls + 1, max_round]
        prob_tail -= prob
    prob_fail += prob_tail
    prob_fail_upperbound = 2 * (1 - (1 - prob_fail) ** n_groups)
    return prob_fail_upperbound


def calc_prob_fail_upperbound(
    total_balls, n_groups, n_bins, max_round, split_num, max_decode_size, round_table
):
    m = min(200, n_bins - 1)
    prob = (1 - 1 / n_groups) ** total_balls
    prob_fail = 0
    prob_tail = 1 - prob
    for n_balls in range(1, max_decode_size):
        prob = prob * (total_balls - n_balls + 1) / n_balls / (n_groups - 1)
        prob_fail += prob * round_table[n_balls + 1, max_round]
        prob_tail -= prob
    for n_balls in range(max_decode_size, m):
        prob = prob * (total_balls - n_balls + 1) / n_balls / (n_groups - 1)
        prob_fail += prob * calc_prob_fail_upperbound_no_split(
            n_balls,
            split_num,
            n_bins,
            max_round - 1,
            split_num,
            max_decode_size,
            round_table,
        )
        prob_tail -= prob

    prob_fail += prob_tail
    prob_fail_upperbound = 2 * (1 - (1 - prob_fail) ** n_groups)
    return prob_fail_upperbound


def get_best_param(diff, avg_diff, max_round, split_num, success_rate):
    min_cost = 1e10
    opt_n = 1
    opt_t = 1
    opt_prob = -1
    for n in range(6, 15):
        t = n
        n_bins = 2 ** n - 1
        m = min(200, n_bins - 1)
        prob = 0
        while prob < success_rate and t < min([200, n_bins - 1, 5 * avg_diff]):
            t += 1
            round_table = get_round(n_bins, m, t, max_round)
            prob = 1 - calc_prob_fail_upperbound(
                diff,
                diff / avg_diff,
                n_bins,
                max_round,
                split_num,
                t,
                round_table,
            )
        if t < min([200, n_bins - 1, 5 * avg_diff]):
            cost = n * t
            if cost < min_cost:
                min_cost = cost
                opt_n = n
                opt_t = t
                opt_prob = prob
    return opt_prob, opt_n, opt_t


def generate_params_file(
    output_name, true_diffs, avg_diff, max_round, split_num, success_rate
):
    if os.path.exists(output_name):
        return
    INF_RATIO = 1.38

    PATH = "../test-sets"
    diffs = []

    for diff in true_diffs:
        filename = f"{PATH}/1000000-1000/diff_estimates_with_tow_{diff}_128_9012.txt"
        estimates = np.loadtxt(filename)
        diffs.extend(np.unique(np.ceil(estimates * INF_RATIO).astype(int)).tolist())

    diffs = list(set(diffs))
    diffs.sort()

    with open(output_name, "w") as f:
        for diff in diffs:
            print(diff)
            result = get_best_param(diff, avg_diff, max_round, split_num, success_rate)
            f.write(f"{diff},{result[0]},{result[1]},{result[2]}\n")


if __name__ == "__main__":
    # balls_into_bins = calc_balls_into_bins(2 ** 6 - 1, 62)
    # transition = calc_transition(balls_into_bins, 7)
    # round_table = calc_round(transition, 3)
    # prob = calc_prob_fail_upperbound(100, 20, 63, 3, 3, 7, round_table)

    # result = get_best_param(100, 5, 3, 3, 0.99)
    # print(result)

    TRUE_DIFFS = [
        10,
        20,
        30,
        60,
        100,
        200,
        400,
        700,
        1000,
        1400,
        2500,
        4000,
        8000,
        10000,
        16000,
        30000,
        50000,
        100000,
    ]

    AVG_DIFFS = [3, 5, 7, 10, 15, 20, 25, 30]

    generate_params_file("params/params_r3_5_all.csv", TRUE_DIFFS, 5, 3, 3, 0.99)

    for avg_diff in AVG_DIFFS:
        generate_params_file(f"params/params_r3_{avg_diff}.csv", [10000], avg_diff, 3, 3, 0.99)

    generate_params_file(
        "params/params_r3_5_vs_graphene_all.csv", TRUE_DIFFS, 5, 3, 3, 1 - 1 / 240
    )
