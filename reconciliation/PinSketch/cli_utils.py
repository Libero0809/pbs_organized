#!/usr/bin/env python3
import json
import os.path
import sys
import click
import numpy as np

STARTER_FILE = "../test-sets/diff_starter.json"
BASH_HEADER = "#!/usr/bin/env bash\n\nset -e\n"
DEFAULT_DIFFS = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000 16000 30000 50000 100000"
RESULT_ROOT_PATH = '../results'

def random_foldername():
    """generate a folder name using current date"""
    from datetime import datetime
    import uuid
    now = datetime.now()
    foldername = now.strftime('%Y-%m-%d') + '-' + str(uuid.uuid4())
    return foldername

def diff_start_gen(diffs):
    if os.path.exists(STARTER_FILE):
        with open(STARTER_FILE, "r") as fp:
            return json.load(fp)
    diff_sizes = [int(s) for s in diffs.split()]
    starter = [0] + list(np.cumsum(diff_sizes)[:-1])
    starter = [int(x) for x in starter]

    j_obj = dict(zip([str(d) for d in diff_sizes], starter))
    with open(STARTER_FILE, "w") as fp:
        json.dump(j_obj, fp, indent=4)
    return j_obj


def pinsketch_experiments(
    binary,
    diffs,
    est_filename,
    union_size,
    input_set_filename,
    num_tests,
    est_inflation_ratio,
    bash_filename,
):
    starter = diff_start_gen(diffs)
    with open(bash_filename, "w") as ofp:
        ofp.write(BASH_HEADER)
        for d in [int(s) for s in diffs.split()]:
            d_str = str(d)
            est_filename_ = est_filename.format(d=d_str)
            command_str = f"{binary} -n {num_tests} -d {d} -s {starter[str(d)]} -u {union_size} -e {est_filename_} -i {input_set_filename} -r {est_inflation_ratio}"
            ofp.write(f'echo "{command_str}"\n')
            ofp.write(f"{command_str}\n")
        result_folder = os.path.join(RESULT_ROOT_PATH,random_foldername())
        ofp.write(f'\necho \"Create new result folder {result_folder} and copy all results to it\"\n')
        ofp.write(f'mkdir -p {result_folder}\n')
        ofp.write(f'cp ./*.txt {result_folder}\n')
        ofp.write(f'echo \"All experiments completed!\"\n')
        return bash_filename

def pinsketch_wp_experiments(
    binary,
    diffs,
    max_rounds,
    best_param_filename_prefix,
    est_filename,
    union_size,
    input_set_filename,
    avg_d_per_group,
    num_tests,
    est_inflation_ratio,
    bash_filename,
):
    starter = diff_start_gen(diffs)
    parameter_filename = f"{best_param_filename_prefix}" 
    #f"{best_param_filename_prefix}{max_rounds}.csv"
    with open(bash_filename, "w") as ofp:
        ofp.write(BASH_HEADER)
        for d in [int(s) for s in diffs.split()]:
            d_str = str(d)
            est_filename_ = est_filename.format(d=d_str)
            for avg_d in [float(s) for s in avg_d_per_group.split()]:
                command_str = f"{binary} -n {num_tests} -d {d} -s {starter[str(d)]} -u {union_size} -e {est_filename_} -i {input_set_filename} -r {est_inflation_ratio} -R {max_rounds} -p {parameter_filename} -m {avg_d}"
                ofp.write(f'echo "{command_str}"\n')
                ofp.write(f"{command_str}\n")
        result_folder = os.path.join(RESULT_ROOT_PATH,random_foldername())
        ofp.write(f'\necho \"Create new result folder {result_folder} and copy all results to it\"\n')
        ofp.write(f'mkdir -p {result_folder}\n')
        ofp.write(f'cp ./*.txt {result_folder}\n')
        ofp.write(f'echo \"All experiments completed!\"\n')
        return bash_filename

CONTEXT_SETTINGS = dict(help_option_names=["-h", "--help"])


@click.group(context_settings=CONTEXT_SETTINGS)
def cli():
    pass

@cli.command(name="pinsketch",context_settings=CONTEXT_SETTINGS)
@click.option(
    "-b", "--binary", required=True, type=str, help="Name of the binary"
)
@click.option(
    "-u", "--union-size", required=True, type=int, help="Size of union"
)
@click.option(
    "-d",
    "--diffs",
    required=True,
    type=str,
    help="Space separated difference sizes",
)
@click.option(
    "-e",
    "--estfilename",
    required=True,
    type=str,
    help="filename for the file storing all estimations",
)
@click.option(
    "-i",
    "--input-sets-filename",
    type=str,
    required=True,
    help="Input sets filename",
)
@click.option(
    "-t", "--num-tests", type=int, default=1000, help="Number of tests"
)
@click.option(
    "-o", "--output", type=str, required=True, help="Output filename"
)
def gen_bash_for_experiments(
    binary,
    union_size,
    diffs,
    estfilename,
    input_sets_filename,
    num_tests,
    output
):
    """Generate bash file for pinsketch experiments"""
    default_est_inflation_ratio = 1.38
    pinsketch_experiments(
        binary,
        diffs,
        estfilename,
        union_size,
        input_sets_filename,
        num_tests,
        default_est_inflation_ratio,
        output
    )

@cli.command(name="pinsketch-wp",context_settings=CONTEXT_SETTINGS)
@click.option(
    "-b", "--binary", required=True, type=str, help="Name of the binary"
)
@click.option(
    "-u", "--union-size", required=True, type=int, help="Size of union"
)
@click.option(
    "-d",
    "--diffs",
    required=True,
    type=str,
    help="Space separated difference sizes",
)
@click.option(
    "-p",
    "--paramfile-prefix",
    required=True,
    type=str,
    help="Prefix of paramfile",
)
@click.option(
    "-e",
    "--estfilename",
    required=True,
    type=str,
    help="filename for the file storing all estimations",
)
@click.option(
    "-g",
    "--avg-diff-per-group",
    default="5",
    type=str,
    help="strings of all average difference sizes to be experimented",
)
@click.option(
    "-i",
    "--input-sets-filename",
    type=str,
    required=True,
    help="Input sets filename",
)
@click.option(
    "-t", "--num-tests", type=int, default=1000, help="Number of tests"
)
@click.option(
    "-r", "--max-rounds", type=int, default=3, help="Maximum number of rounds"
)
@click.option(
    "-o", "--output", type=str, required=True, help="Output filename"
)
def gen_bash_for_wp_experiments(
    binary,
    union_size,
    diffs,
    paramfile_prefix,
    estfilename,
    avg_diff_per_group,
    input_sets_filename,
    num_tests,
    max_rounds,
    output
):
    """Generate bash file for pinsketch-wp experiments"""
    default_est_inflation_ratio = 1.38
    pinsketch_wp_experiments(
        binary,
        diffs,
        max_rounds,
        paramfile_prefix,
        estfilename,
        union_size,
        input_sets_filename,
        avg_diff_per_group,
        num_tests,
        default_est_inflation_ratio,
        output
    )


if __name__ == "__main__":
    cli()
