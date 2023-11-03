### Enable compiler warnings and sanitizers

Added the following flags to the compiler call `-Wall -Wextra -pedantic` in the `Makefile`.

This results in the following compiler output:

```
example_1.c: In function ‘main’:
example_1.c:13:9: warning: unused variable ‘i’ [-Wunused-variable]
   13 |     int i;
      |         ^
example_2.c: In function ‘main’:
example_2.c:32:28: warning: operation on ‘rank’ may be undefined [-Wsequence-point]
   32 |                      (rank = 1 + size) % size, 123, MPI_COMM_WORLD, &status);
      |                      ~~~~~~^~~~~~~~~~~
```

In order to fix the first warning we can just remove the unused variable on line 13.

To fix the second warning we can take the comment above into account which states this should be a ring communication which suggests in this case we would like to get the rank id of the previous rank which would mean this segment should be changed to `rank - 1 + size`.

We can see the same segment on the `MPI_Recv` call below but this does not lead to the warning but the incorrect output at the end of the program:
`Signing off, rank 5.`

So this segment should be changed to `rank - 1 + size` aswell.
This will get the program to work but strictly speaking this is not a totally correct MPI program since the `MPI_Send` call could be blocking and then the `MPI_Recv` is never reached and we end up in a deadlock.
To correct this the `MPI_Send` should be changed to a `MPI_Isend`

Another mistake was the recieve datatype in the `MPI_Sendrecv` which is set to `MPI_BYTE` but since we recieve 2 Integers this should be changed to `MPI_INT`.

#### Sanitizers

In order to enable Sanitizers `-fsanitize=undefined,address` was added to the compiler flags in the `Makefile` but after runing the executable this did not lead to any warnings from the sanitizers on the first execution.

But upon a quick look into `example_1.c`, we found that the `MPI_Send` call uses `tag` and the `MPI_Recv` call uses `tag2` and after changing the recieve call to `tag` we can observe the following sanitizers output.

```
=================================================================
==9289==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 4816 byte(s) in 7 object(s) allocated from:
    #0 0x7f86afee0cc1 in __interceptor_calloc /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_malloc_linux.cpp:77
    #1 0x7f86afd9baed in ompi_op_base_op_select (/usr/lib/libmpi.so.40+0xc6aed) (BuildId: 3fbe04cc3371bb1fdfa6c97ee6b6bdc5b55b5f66)

Direct leak of 4816 byte(s) in 7 object(s) allocated from:
    #0 0x7f86afee0cc1 in __interceptor_calloc /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_malloc_linux.cpp:77
    #1 0x7f86ae708430  (<unknown module>)

Direct leak of 2048 byte(s) in 1 object(s) allocated from:
    #0 0x7f86afee1359 in __interceptor_malloc /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x7f86af2b495f in opal_dss_buffer_extend (/usr/lib/libopen-pal.so.40+0x2f95f) (BuildId: bc080bdea00247860a3faf046f73e74047ea57cf)

Direct leak of 290 byte(s) in 7 object(s) allocated from:
    #0 0x7f86afee1359 in __interceptor_malloc /usr/src/debug/gcc/gcc/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x7f86af49f1b0  (/usr/lib/libc.so.6+0x811b0) (BuildId: 8bfe03f6bf9b6a6e2591babd0bbc266837d8f658)
```

But after inspection this does not show any leaks in our code so this output is not really useful for debugging.

### MUST

To use `MUST` we just simply changed the `job.sh` file to not use `mpirun` but use the provided `mustrun`:
(Note that we have to specify `SLURM_NTASKS-1` because 1 extra process is used by `MUST`)

```
/scratch/c703429/software/must-1.9.1/bin/mustrun -np $(($SLURM_NTASKS-1)) ./a.out
```

In the initial run without changing anything this did not lead to any useful output.

Unfortunately `MUST` was not really working for us since no matter the program the ouput always hang forever at `Executing application:`:

```
Using prebuild /home/cb76/cb761032/.cache/must/prebuilds/b62ed64425437f6c696454be27e2071b
Using prebuild /home/cb76/cb761032/.cache/must/prebuilds/b62ed64425437f6c696454be27e2071b
[MUST] MUST configuration ... centralized checks with fall-back application crash handling (very slow)
[MUST] Information: overwritting old intermediate data in directory "/home/cb76/cb761032/must_temp"!
[MUST] Using prebuilt infrastructure at /home/cb76/cb761032/.cache/must/prebuilds/b62ed64425437f6c696454be27e2071b
[MUST] Weaver ... success
[MUST] Generating P^nMPI configuration ... success
[MUST] Search for linked P^nMPI ... not found ... using LD_PRELOAD to load P^nMPI ... success
[MUST] Executing application:
```

### Old programs

Unfortunately all old programs which did not really work were eventully deleted and we also could not really get `MUST` to run while complier flags were already used before this exercise.
