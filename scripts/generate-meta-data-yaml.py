# Script to generate the meta data file that is stored with each experiment.
#
# Meta data file definition: (these fields are required)
# general:
# - ID
# - Date time
# - Platform
# - Node_list
# - number of Nodes
# - number of tasks
# 
# benchmark:
# - name
# - version
# - git commit id
# - git origin
# 
# hemocell:
# - version
# - git link
# - git commit
# 
# ear:
# - policy
# - files

import argparse
import os
import subprocess
import yaml
from datetime import datetime


def run_shell_cmd(command, cwd='./'):
    return subprocess.run(command.split(' '), stdout=subprocess.PIPE, cwd=cwd).stdout.decode('utf-8')

def gen_hemocell(dir_name):
    """
    Get the hemocell information.
    Info is fetched from the git of the given directory.
    """
    d = {}

    d['git_commit'] = run_shell_cmd("git rev-parse HEAD", dir_name).strip()
    d['git_origin'] = run_shell_cmd("git ls-remote --get-url origin", dir_name).strip()
    d['version'] = run_shell_cmd("git tag -l", dir_name).strip().split('\n')[-1]

    return d

def gen_ear():
    
    d = {}

    d['policy'] = "monitoring"

    return d

def gen_general(args):

    now = datetime.now()

    d = {}

    d['id'] = args.id
    d['platform'] = args.platform
    d['date'] = now.strftime("%Y-%m-%d %H:%M")
    d['node_list'] = os.getenv("SLURM_JOB_NODELIST")
    d['tasks'] = os.getenv("SLURM_NTASKS")
    d['nodes'] = os.getenv("SLURM_NNODES")

    return d

def gen_benchmark(dir_name):
    """
    Get the benchmakr information.
    Info is fetched from the git of the given directory and the meta.yml file located in the benchmark dir.
    """

    d = {}
    with open(dir_name + '/meta.yml') as f:
        d = yaml.load(f, Loader=yaml.FullLoader)

    d['git_commit'] = run_shell_cmd("git rev-parse HEAD", dir_name).strip()
    d['git_origin'] = run_shell_cmd("git ls-remote --get-url origin", dir_name).strip()

    return d


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("hemocell_dir", type=str, help="Hemocell directory")
    parser.add_argument("benchmark_dir", type=str, help="Benchmark directory")
    parser.add_argument("-p", "--platform", type=str, help="Which platform are we running on")
    parser.add_argument("-i", "--id", type=str, help="Experiment id")
    parser.add_argument("-f", "--out_file", type=str, help="name of the output file", default="meta.yml")
    args = parser.parse_args()

    meta_dict = {}
    meta_dict['general'] = gen_general(args);
    meta_dict['benchmark'] = gen_benchmark(args.benchmark_dir);
    meta_dict['hemocell'] = gen_hemocell(args.hemocell_dir);
    meta_dict['ear'] = gen_ear();
    meta_dict_yaml = yaml.dump(meta_dict, sort_keys=False)

    with open(args.out_file, "w") as text_file:
        text_file.write(meta_dict_yaml)

if __name__ == "__main__":
    main()
