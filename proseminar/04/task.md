## 1D Heat Stencil

#### perf stat

This is the output of `perf stat` of one node when running the 1D Heat stencil with `N=2048` and 4 ranks.
```
Performance counter stats for './heat_stencil_1D_mpi 2048':

            321.03 msec task-clock:u              #    0.623 CPUs utilized          
                 0      context-switches:u        #    0.000 /sec                   
                 0      cpu-migrations:u          #    0.000 /sec                   
             2,487      page-faults:u             #    7.747 K/sec                  
       822,551,948      cycles:u                  #    2.562 GHz                      (83.96%)
       355,917,160      stalled-cycles-frontend:u #   43.27% frontend cycles idle     (83.98%)
       197,555,946      stalled-cycles-backend:u  #   24.02% backend cycles idle      (65.27%)
     1,520,524,739      instructions:u            #    1.85  insn per cycle         
                                                  #    0.23  stalled cycles per insn  (82.64%)
       276,487,757      branches:u                #  861.246 M/sec                    (83.61%)
         1,313,382      branch-misses:u           #    0.48% of all branches          (83.21%)

       0.515709212 seconds time elapsed

       0.276931000 seconds user
       0.045040000 seconds sys
```

#### perf record

When profiling the program with `perf record` and the analyzing with `perf reprot` we can observe the following output:
```
Samples: 2K of event 'cycles:u', Event count (approx.): 1601872747
Overhead  Symbol                                        Source:Line
  35.01%  [.] main                                      stencil-1D.c:126
  15.88%  [.] 0x000000000000a706                        libmlx4-rdmav2.so[a706]
   7.21%  [.] main                                      stencil-1D.c:112
   4.25%  [.] pthread_spin_lock                         pthread_spin_lock+4
   4.04%  [k] 0xffffffffb7a001a5                        ??:0
   2.33%  [.] 0x0000000000025bf2                        libuct_ib.so.0.0.0[25bf2]
   1.12%  [.] 0x0000000000025c2c                        libuct_ib.so.0.0.0[25c2c]
   1.08%  [k] 0xffffffffb7a00b87                        ??:0
...
```

If we exampine line 126:
```
			B[i] = tc + 0.2 * (tl + tr + (-2 * tc));
```
We can see that most of the time is spent executing the caluclation of temperature of each cell.
Follwing is a part int `libmlx4-rdmav2.so` which is a library for Infiniband communication suggesting this is some sort of communication with other ranks.

Afterwards with 7% follows the soure line 112 which again is part of the loop calculating the cell temperatures.

#### prof and gprof

**Unfortunately due to an incompatible dwarf version we were no able to execute these profiling methods on the lcc3 cluster. The following profiles were performed on a local machine which limits there usefulness but since performance sould only be relative this still gives a good indication of how the progam overall behaves and where the hotspots are located.**

![gpref output](./images/1d.png)

If we inspect the output of gperf we can see again that a large portion of the time ist spent in the functions `writev` and `readv` which indicates most of the time is spent writing and reading from the MPI buffers.

## 2D Heat Stencil


# Optimizations
In the process of analyzing and optimizing the 2D heat stencil application, we observed that a considerable portion of the runtime is consumed by MPI communication. To address this performance bottleneck, the primary strategy for optimization was to leverage MPI's internal functionalities specifically designed for enhancing communication efficiency. By initially employing MPI_Cart_create for optimal rank placement and subsequently adopting MPI_Neighbors_alltoallw for streamlined halo exchange, significant improvements in performance were achieved.

#### Optimize Rank Placement with MPI_Cart_create

Before diving into halo exchange optimizations, the first step taken was to utilize `MPI_Cart_create` for optimized placement of ranks. This function allows you to create a Cartesian topology and enables MPI to rearrange ranks if necessary, to minimize communication cost. 

The `MPI_Cart_create` function helps in logically arranging the ranks in a grid format that closely mirrors the 2D stencil, thereby potentially optimizing the communication between adjacent ranks. This optimized placement often leads to better cache locality and reduced communication time, especially in large, multi-node systems.

#### optimizing Halo Exchange

1. **Replacing Manual Isend and Irecv with MPI_Neighbors_alltoallw**: After optimizing rank placement, the next step is to use `MPI_Neighbors_alltoallw` to further improve the data exchange by allowing MPI to optimize it automatically.

2. **Overlapping Communication and Computation**: Along with the above, you can also employ latency hiding. Start the asynchronous communication and then proceed with computations for the inner cells. Once the communication is complete, update the boundary cells.

## Final Walltime and Speedup

This resulted in the following Wall time:
`1.376 seconds` for N=768 and T=76800

Considering the Wall time of the sequential version: `122.895` we arrive at a speedup of: `89.313`
