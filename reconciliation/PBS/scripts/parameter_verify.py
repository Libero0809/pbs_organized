#!/usr/bin/env python3
import numpy as np


def existing_verify():
    diffs = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000"
    for r in [2, 3]:
        data = np.loadtxt(f'params_r{r}.csv', delimiter=',')
        dd = set([int(d) for d in data[:, 0]])
        for d in [int(ds) for ds in diffs.split()]:
            if not (d in dd):
                print("Error")


if __name__ == '__main__':
    existing_verify()
