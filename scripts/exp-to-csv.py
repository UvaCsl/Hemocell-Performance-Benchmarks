#! python3

import numpy as np
import argparse
import os
import subprocess
import sys
import fileinput
import pandas as pd
from pycubexr import CubexParser

def run_shell_cmd(command, retstatus=False, cwd='./'):
    tmp = subprocess.run(command, stdout=subprocess.PIPE, cwd=cwd,shell=True)
    
    if retstatus:
        return tmp.stdout.decode('utf-8'), tmp.returncode
    else:
        return tmp.stdout.decode('utf-8')

def parse_exp(args, exp_dir):
    print("------ ENTERING {} ------".format(exp_dir))
    scoredir=run_shell_cmd("ls -d {}/*SCOREP*/".format(exp_dir)).strip()
    cubexfile=scoredir + args.cubexname + ".cubex"
    csvfile=exp_dir + args.outputname + ".csv"

    to_generate_or_not_to_generate = {"csv":True, "cubex":True} # Store if we need to generate these
    if os.path.isfile(csvfile):
        to_generate_or_not_to_generate["csv"] = False
    if os.path.isfile(cubexfile):
        to_generate_or_not_to_generate["cubex"] = False

    if not to_generate_or_not_to_generate['csv']: # Check if we have to generate a csv file for this experiment
        print("{} already exists.".format(csvfile))
        if not args.force:
            print("Skipping experiment")
            return

        to_generate_or_not_to_generate['csv'] = True
        print("Force is set, generating csv file anyway")

    if not to_generate_or_not_to_generate['cubex']: # Check if we need to generate cubex file
        print("{} already exists.".format(cubexfile))
        if args.force:
            print("Force is set, generating cubex file anyway")
            to_generate_or_not_to_generate['cubex'] = True
        print()

    parse_cubex_to_csv(args, scoredir, cubexfile, csvfile, to_generate_or_not_to_generate)

    if to_generate_or_not_to_generate["csv"]:
        update_csv(args, scoredir, cubexfile, csvfile)


def parse_cubex_to_csv(args, scoredir, cubexfile, csvfile, gen):
    print("Parsing SCOREP results")

    if gen['cubex']:
        print("Generating {}".format(cubexfile))
        run_shell_cmd("square -s {}".format(scoredir)) # Let scalasca parse the cubexfile into summary.cubex
        run_shell_cmd("cube_canonize -cfl {}/summary.cubex {}".format(scoredir, cubexfile)) # Clean the text in the cubex files

        _, retcode = run_shell_cmd("cube_calltree {} | grep -q slice".format(cubexfile), True) # Check if the iteration-bins are present in the cube file
        if retcode == 0: # Reroot the cubex file to the imporant code sections
            run_shell_cmd("cube_cut -r \"slice\" -o {} {}".format(cubexfile, cubexfile)) 
        else:
            _, retcode2 = run_shell_cmd("cube_calltree {} | grep -q \"{}\"".format(cubexfile, args.cutpoint), True) # Check if the iteration-bins are present in the cube file

            if retcode2 == 0:
                run_shell_cmd("cube_cut -r \"{}\" -o {} {}".format(args.cutpoint, cubexfile, cubexfile))

    if gen['csv']:
        print("Writing CSV")
        run_shell_cmd("cube_dump -m comp,mpi,execution -s csv2 -z incl -o {} {}".format(csvfile, cubexfile)) # Generate csv from cubexfile

def clean_region_name(name):
    
    if '(' in name:
        name = name[:name.find('(')]

    if ':' in name:
        name = name[name.rfind(':') + 1: ]

    return name

def update_csv(args, scoredir, cubexfile, csvfile):
    print("Updating CSV")

    df = pd.read_csv(csvfile, dtype={'Cnode ID': int})
    cnodes = np.unique(np.array(df['Cnode ID']))
    names = [None] * (np.max(cnodes)+1)
    iteration = [None] * (np.max(cnodes)+1)
    i = 0
    firstIter = True 

    with CubexParser(cubexfile) as cubex:
        for c in cnodes:
            names[c] = clean_region_name(cubex.get_region(cubex.get_cnode(c)).name)

            if "iteration=" in names[c]:
                if firstIter:
                    firstIter=False
                else:
                    i = i + 1

            iteration[c] = i

    df["regionName"] = df.apply(lambda row: names[int(row["Cnode ID"])], axis=1)
    df["iteration"]  = df.apply(lambda row: iteration[int(row["Cnode ID"])], axis=1)

    df.to_csv(csvfile, index=False)

def loop_through_exps(args):
    dirs = run_shell_cmd("ls -d {}/*/".format(args.expdir)).strip().split('\n')

    for exp_dir in dirs:
        parse_exp(args, exp_dir)


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("expdir", type=str, help="Directory where all of the experiment dirs are located. Each experiment folder should contain a subfolder with SCOREP in the name where the SCOREP results are located.")
    parser.add_argument("-o", "--outputname", type=str, help="Name of the .csv and .cubex output file.", default="results")
    parser.add_argument("-c", "--cubexname", type=str, help="Name of the .cubex output file, default is same as --ouptut option.", default=None)
    parser.add_argument("-s", "--single", action='store_true', help="Set this flag if the given directory is a specific experiment.")
    parser.add_argument("-f", "--force", action='store_true', help="If forces is set it forces the script to regenerated already existing cubex and csv files.")
    parser.add_argument('--cutpoint', type=str, help="Set a different cutpoint for the cubexfile, if the cutpoint does not exist cubexfile will not be cut", default="void hemo::hemocell::iterate")
    args = parser.parse_args()

    if args.cubexname is None:
        args.cubexname = args.outputname

    if args.single:
        parse_exp(args, args.expdir)
    else:
        loop_through_exps(args)

    return


if __name__ == "__main__":
    main()
