#!/bin/bash 
 
# Execute job in the partition "lva" unless you have special requirements. 
#SBATCH --partition=lva 
# Name your job to be able to identify it later 
#SBATCH --job-name particle
# Redirect output stream to this file 
#SBATCH --output=output.log 
# Enforce exclusive node allocation, do not share with other jobs 
#SBATCH --exclusive 
~/uibk_hpc_23/proseminar/06/nbody
