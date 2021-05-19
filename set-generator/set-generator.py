#!/usr/bin/env python3

import numpy as np
import click
import typing as t
from datetime import datetime


def generate_sets(
    size: int, how_many: int, elm_length: 32, seed: t.Optional[int] = None
) -> np.ndarray:
    """Generate sets

    Arguments:
        size {int} -- the union size of each set pair
        how_many {int} -- # of pairs to be generated
        elm_length {32} -- length of each element in the set

    Keyword Arguments:
        seed {t.Optional[int]} -- random seed (default: {None})

    Returns:
        sets {numpy.ndarray} -- generated sets
    """

    blk_size = 100
    num_blks = int(how_many / blk_size)
    ratio = 1.1
    dtype = np.int32 if elm_length <= 32 else np.int64

    if seed:
        np.random.seed(seed)

    sets_a = np.zeros((how_many, size))

    k = 0
    for i in range(num_blks):
        data = np.random.randint(
            low=1,
            high=np.iinfo(dtype).max,
            size=(num_blks, int(ratio * size)),
            dtype=dtype,
        )
        for raw_set in data:
            s = np.unique(raw_set)
            if len(s) < size:
                padding = np.random.randint(
                    low=1,
                    high=np.iinfo(dtype).max,
                    size=(int(3 * (size - len(s)))),
                    dtype=dtype,
                )
                s = np.unique(np.concatenate((s, padding), axis=None))
            assert len(s) >= size
            sets_a[k, :] = np.random.permutation(s[:size])
            k += 1

    if k < how_many:
        data = np.random.randint(
            low=1,
            high=np.iinfo(dtype).max,
            size=(how_many - k, int(ratio * size)),
            dtype=dtype,
        )
        for raw_set in data:
            s = np.unique(raw_set)
            if len(s) < size:
                padding = np.random.randint(
                    low=1,
                    high=np.iinfo(dtype).max,
                    size=(int(3 * (size - len(s)))),
                    dtype=dtype,
                )
                s = np.unique(np.concatenate((s, padding), axis=None))
            assert len(s) >= size
            sets_a[k, :] = np.random.permutation(s[:size])
            k += 1

    return sets_a


CONTEXT_SETTINGS = dict(help_option_names=["-h", "--help"])


@click.command(context_settings=CONTEXT_SETTINGS)
@click.option("-s", "--size", default=100000, help="Size (cardinality) of set.")
@click.option("-n", "--number-sets", default=1000000, help="Number of sets.")
@click.option(
    "-l",
    "--element-length",
    type=click.Choice(["32", "64"]),
    default="32",
    help="Universe size.",
)
@click.option("--seed", type=int, help="Seed.")
@click.option("-o", "--output", type=click.Path(exists=False), help="Output filename.")
def generate_sets_cli(
    size: int, number_sets: int, element_length: str, seed: int, output: str
):

    if not seed:
        seed = int(datetime.now().timestamp())
    if not output:
        output = f"sets_{size}_{number_sets}_{element_length}_{seed}.txt"

    with open(output, "w") as fp:
        each = 1000
        remaining = number_sets

        fp.write("# union_size number_sets size_universe\n")
        fp.write(f"{size} {number_sets} {element_length}")
        fp.write("\n")
        fp.write("# sets (one per line)\n")

        print(f"Starting to generate ...")
        while remaining >= each:
            sets = generate_sets(
                size=size, how_many=each, elm_length=int(element_length), seed=seed
            )
            np.savetxt(fp, sets, fmt="%i", delimiter=" ")
            remaining -= each
            print(f"{number_sets - remaining}/{number_sets} ...")
        if remaining > 0:
            sets = generate_sets(
                size=size, how_many=remaining, elm_length=int(element_length), seed=seed
            )

            np.savetxt(fp, sets, fmt="%i", delimiter=" ")
        print(f"Done!")


if __name__ == "__main__":
    generate_sets_cli()
