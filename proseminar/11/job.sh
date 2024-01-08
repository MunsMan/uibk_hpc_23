#!/bin/bash

# Execute job in the partition "lva" unless you have special requirements.
#SBATCH --partition=lva
# Name your job to be able to identify it later
#SBATCH --job-name cpl-monte 
# Redirect output stream to this file
#SBATCH --output=output.log
# Maximum number of tasks (=processes) to start in total
#SBATCH --ntasks=12
# Enforce exclusive node allocation, do not share with other jobs
#SBATCH --exclusive

/scratch/cb761032/chapel-1.33.0/cpl-monte
