import argparse
import logging
import shutil
import os
import subprocess
import yaml
import numpy as np
from datetime import datetime

def run_shell_cmd(command, cwd='./'):
    return subprocess.run(command.split(' '), stdout=subprocess.PIPE, cwd=cwd).stdout.decode('utf-8')

RBC_VOLUME = 90


# https://stackoverflow.com/questions/15347174/python-finding-prime-factors
def prime_factors(n):
    i = 2
    factors = []
    while i * i <= n:
        if n % i:
            i += 1
        else:
            n //= i
            factors.append(i)
    if n > 1:
        factors.append(n)
    return factors


class Experiment:
    # input_dir = "./"
    # output_dir = "./"
    # name = ""
    # processes = None
    # size = None
    # ab_size = None
    FILES=["RBC.xml", "RBC.pos", "PLT.xml", "PLT.pos", "config.xml", "filter.filter"]

    def __init__(self, name, input_dir, output_dir, np, size, ab_size, iterations):
        self.name = name
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.np = np 
        self.size = size
        self.ab_size = ab_size
        self.iterations = iterations
        self.experiment_dir = f"{self.output_dir}/{self.name}"
        self.config_file = f"{self.output_dir}/{self.name}/config.xml"
        self.RBC_size = [8, 4, 8]


    @classmethod
    def from_args(cls, parser):
        """Make an experiment from cmd line arguments"""
        ab_size = parser.atomic_block_size
        size = parser.size

        if not size is None:
            size = tuple([int(x) for x in size.split(',')])
        if not ab_size is None:
            ab_size = tuple([int(x) for x in ab_size.split(',')])

        return cls(parser.name, parser.input_dir, parser.output_dir, parser.np, size, ab_size, parser.iterations)


    def write_to_config(self, root, field, value):
        """
        Write {value} to the {field} line of {self.config_file}.
        If {field} not found, its added within the {root} tag, if {root} tag not found it is added to the <hemocell> tag
        """

        new_line = f"\t<{field}> {value} </{field}>\n"
        with open(self.config_file, 'r', encoding='utf-8') as file:
            data = file.readlines()

        replaced = False
        for i, line in enumerate(data):
            if f"<{field}>" in line:
                data[i] = new_line
                replaced = True
                break

        if not replaced:
            for i, line in enumerate(data):
                if f"<{root}>" in line:
                    data.insert(i+1, new_line)
                    replaced = True
                    break

        if not replaced:
            data.insert(len(data) - 1, f"<{root}>\n")
            data.insert(len(data) - 1, new_line)
            data.insert(len(data) - 1, f"</{root}>\n\n")


        with open(self.config_file, 'w', encoding='utf-8') as file: 
            file.writelines(data)


    def set_ab_size(self):
        """ Set self.ab_size """
        if not self.ab_size is None:
            return

        self.ab_size = [0, 0, 0]
        primes = prime_factors(self.np) 
        pr = [1,1,1]

        i = 0
        primes.reverse()
        for x in primes:
            pr[i] = pr[i] * x
            i = (i + 1) % 3

        self.ab_size[0] = self.size[0] / pr[0]
        self.ab_size[1] = self.size[1] / pr[1]
        self.ab_size[2] = self.size[2] / pr[2]


    def set_size(self):
        """ Write new size to config. """
        size = None

        if not self.size is None:
            size = self.size

        elif not self.ab_size is None and not self.np is None:
            primes = prime_factors(self.ab_size[0] * self.ab_size[1] * self.ab_size[2] * self.np) 
            size = [1,1,1]

            i = 0
            primes.reverse()
            for x in primes:
                size[i] = size[i] * x
                i = (i + 1) % 3

        if not size is None:
            self.size = size
            self.write_to_config("domain", "nx", size[0])
            self.write_to_config("domain", "ny", size[1])
            self.write_to_config("domain", "nz", size[2])


    def create(self):
        try:
            os.mkdir(f"{self.experiment_dir}")
            logging.info(f"Experiment: {self.experiment_dir} folder created")
        except FileExistsError:
            logging.warning(f"Experiment '{self.experiment_dir}' already exists.")
        except PermissionError:
            logging.warning(f"Permission denied: Unable to create '{self.experiment_dir}'.")
        except Exception as e:
            logging.warning(f"An error occurred: {e}")

        run_shell_cmd(f"cp {' '.join([self.input_dir + '/' + f for f in self.FILES])} {self.experiment_dir}")

        self.write_to_config("sim", "tmax", self.iterations)

        self.set_size()
        self.set_ab_size()


class FractionalImbalance(Experiment):

    blocks = []

    def __init__(self, name, input_dir, output_dir, np, size, ab_size, iterations, fli_fluid, fli_part, fli_part_base, fli_part_stack, eli_case):
        super().__init__(name, input_dir, output_dir, np, size, ab_size, iterations)
        self.fli_fluid = fli_fluid
        self.fli_part  = fli_part
        self.fli_part_base = fli_part_base
        self.fli_part_stack = fli_part_stack
        self.eli_case = eli_case

    def create(self):
        super().create()
        self.fli()
        # Change fli fluid parameters
        # Create RBC.plt based on fli


    def fli_part_stacked(self):
        si = np.array(self.size).argmax()

        total = self.fli_part_base * self.np
        peak = (int(self.fli_part_base * (self.fli_part + 1)), int(self.np * .1))
        base = int((total - (peak[0] * peak[1])) / (self.np - peak[1]))
        left = total - (base * (self.np - peak[1]) + (peak[0] * peak[1]))
        RBCs = []
        

        if self.eli_case == 3:
            for b in self.blocks:
                for _ in range(b[3]):
                    RBCs.append(f"{b[0]/2 + 5:.1f} {b[1]/2 + 5:.1f} {b[2]/2 + 5:.1f} 0 0 0\n")

        else:
            i = 0
            for b in self.blocks:
                if i < peak[1]:
                    n = peak[0]
                else:
                    n = base
                    if i - peak[1] < left:
                        n = n + 1

                for _ in range(n):
                    RBCs.append(f"{b[0]/2 + 5:.1f} {b[1]/2 + 5:.1f} {b[2]/2 + 5:.1f} 0 0 0\n")

                i = i + 1

        return RBCs

    def fli_part_non_stacked(self):
        primes = prime_factors(self.np) 
        pr = [1,1,1]

        i = 0
        primes.reverse()
        for x in primes:
            pr[i] = pr[i] * x
            i = (i + 1) % 3

        ab_size  = self.ab_size
        RBC_size = self.RBC_size

        total = self.fli_part_base * self.np
        peak = (int(self.fli_part_base * (self.fli_part + 1)), int(self.np * .1))
        base = int((total - (peak[0] * peak[1])) / (self.np - peak[1]))
        left = total - (base * (self.np - peak[1]) + (peak[0] * peak[1]))
        RBCs = []

        l_index = np.argsort(self.size)
        max_cells = [int((self.ab_size[0] - 10) / (self.RBC_size[0] * 2)), 
                     int((self.ab_size[1] - 5)  / (self.RBC_size[1] * 2)), 
                     int((self.ab_size[2] - 10) / (self.RBC_size[2] * 2))] 

        i = 0
        # Loop through each process
        for x in range(pr[0]):
            for y in range(pr[1]):
                for z in range(pr[2]):
                    if i < peak[1]:
                        n = peak[0]
                    else:
                        n = base
                        if i - peak[1] < left:
                            n = n + 1

                    if i < peak[1]:
                        n = peak[0]
                    else:
                        n = base
                        if i - peak[1] < left:
                            n = n + 1
                    i = i + 1

                    num_cells = n

                    if num_cells > max_cells[0] * max_cells[1] * max_cells[2]:
                        print("WARNING: Atomic-block not large enough for number of cells")

                    ic = 0
                    offset = [0.5 * x * ab_size[0] + 10, 0.5 * y * ab_size[1] + 5, 0.5 * z * ab_size[2] + 10]


                    # Loop through each cell in a AB
                    for ix in range(max_cells[0]):
                        for iy in range(max_cells[1]):
                            for iz in range(max_cells[2]):
                                l_offset = [ix * RBC_size[0], iy * RBC_size[1], iz * RBC_size[2]]
                                RBCs.append(f"{offset[0] + l_offset[0]:.1f} {offset[1] + l_offset[1]:.1f} {offset[2] + l_offset[2]:.1f} 0 0 0\n")
                                ic += 1
                                if ic == num_cells:
                                    break

                            if ic == num_cells:
                                break

                        if ic == num_cells:
                            break

    def fli_create_part(self):
        """ Create the RBC.pos files """

        self.write_to_config("benchmark", "FLIpart", self.fli_part)


        # Create RBC array if RBCs are stacked
        if self.fli_part_stack:
            RBCs = self.fli_part_stacked()
        else:
            RBCs = self.fli_part_non_stacked()


        RBCs.insert(0, f"{len(RBCs)}\n")

        f = open(f"{self.experiment_dir}/RBC.pos", "w")
        f.write("".join(RBCs))
        f.close()


    def fli_create_blocks(self):
        primes = prime_factors(self.np) 
        pr = [1,1,1]

        i = 0
        primes.reverse()
        for x in primes:
            pr[i] = pr[i] * x
            i = (i + 1) % 3

        ab_size  = self.ab_size

        si = np.array(self.size).argmax()
        ab_size_large = list(ab_size)
        ab_size_large[si] = int(ab_size_large[si] * (self.fli_fluid + 1))
        
        v_large = ab_size_large[0]/2 * ab_size_large[1]/2 * ab_size_large[2]/2
        RBCs_large = int((v_large / 100 * self.fli_part_base) / RBC_VOLUME)

        ab_size_small = list(ab_size)
        ab_size_small[si] = int((self.size[si] - ab_size_large[si]) / (pr[si] - 1))
        v_small = ab_size_small[0]/2 * ab_size_small[1]/2 * ab_size_small[2]/2
        RBCs_small = int((v_small / 100 * self.fli_part_base) / RBC_VOLUME)

        pr_cpy = [pr[0], pr[1], pr[2]]
        pr_cpy[si] = 1

        x = 0
        y = 0
        z = 0

        for xi in range(pr_cpy[0]):
            for yi in range(pr_cpy[1]):
                for zi in range(pr_cpy[2]):
                    self.blocks.append((xi * ab_size_large[0], yi * ab_size_large[1], zi * ab_size_large[2], RBCs_large))

        tmp = [0,0,0]
        tmp[si] = 1
        pr_cpy = [pr[0], pr[1], pr[2]]
        pr_cpy[si] = pr[si] - 1
        for xi in range(pr_cpy[0]):
            for yi in range(pr_cpy[1]):
                for zi in range(pr_cpy[2]):
                    self.blocks.append(((xi * ab_size_small[0]) + (tmp[0] * ab_size_large[0]),
                                        (yi * ab_size_small[1]) + (tmp[1] * ab_size_large[1]),
                                        (zi * ab_size_small[2]) + (tmp[2] * ab_size_large[2]),
                                        RBCs_small))




    def fli(self):
        """ Update the config and create RBC set fli"""

        #FLI fluid
        self.write_to_config("benchmark", "FLIfluid", self.fli_fluid)

        self.fli_create_blocks()

        #skip rest if not fli_part set
        if self.fli_part is None:
            return
        
        self.fli_create_part()




    @classmethod
    def from_experiment(cls, exp, fli_fluid, fli_part, fli_part_base, fli_part_stack, eli_case):
        return cls(exp.name, exp.input_dir, exp.output_dir, 
                   exp.np, exp.size, exp.ab_size, exp.iterations, 
                   fli_fluid, fli_part, fli_part_base, fli_part_stack, eli_case)

    @classmethod
    def from_args(cls, parser):
        """Make an experiment from cmd line arguments"""

        obj = Experiment.from_args(parser)

        return cls.from_experiment(obj, parser.fli_fluid, parser.fli_part, parser.fli_part_base, parser.fli_part_stack, parser.ELI_case)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input_dir", type=str, help="Location of all input files.", default="./")
    parser.add_argument("-o", "--output_dir", type=str, help="Location of output directory.", default="./")
    parser.add_argument("-n", "--name", type=str, help="Name of the experiment", default=None)
    parser.add_argument("-p", "--np", type=int, help="Number of processes", default=None)
    parser.add_argument("-s", "--size", type=str, help="Size of domain, format=x,y,z", default=None)
    parser.add_argument("-a", "--atomic_block_size", type=str, help="Size of atomic block, format=x,y,z", default=None)
    parser.add_argument("-e", "--experiment", type=str, help="Name of the experiment", default="cube")
    parser.add_argument("-t", "--iterations", type=int, help="Number of iterations", default=1)
    parser.add_argument("--fli_fluid", type=float, help="Fractional imbalance fluid", default=None)
    parser.add_argument("--fli_part", type=float, help="Fractional imbalance part", default=0)
    parser.add_argument("--fli_part_base", type=int, help="Fractional imbalance part", default=0)
    parser.add_argument("--fli_part_stack", type=bool, help="If true RBCs are stacked", default=False)
    parser.add_argument("--ELI_case", type=int, help="Case type from embracing load imbalance paper cases", default=1)

    #Still need to setup
    parser.add_argument("-v", "--log", type=str, help="Loggin level=[DEBUG,INFO,WARNING,ERROR,CRITICAL]", default="WARNING")

    args = parser.parse_args()

    exp = FractionalImbalance.from_args(args)
    exp.create()

if __name__ == "__main__":
    main()
