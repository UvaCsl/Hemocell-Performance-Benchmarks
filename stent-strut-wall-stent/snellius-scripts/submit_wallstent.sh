#!/bin/bash
#SBATCH --partition thin
#SBATCH -J stent_strut_wall
#SBATCH --nodes 50
#SBATCH --tasks-per-node 128
#SBATCH --exclusive
#SBATCH --time 2-04:00:00 #runtime

# load modules for hemocell
source "../../scripts/snellius_env.sh"

#srun works better with the environment than mpirun
srun ./stent_strut config_wallstent.xml