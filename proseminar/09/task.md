# Task 1

Laptop CPU: `11th Gen Intel(R) Core(TM) i7-11370H @ 3.30GHz`

The Intel Core i7-11370H supports basic Intel RAPL features. This includes monitoring the power consumption of the CPU cores and the whole CPU Package.

### Gathering information

In order to gather more information about the underlying power instrumentation a program that reads the MSRs was used from ![rapl-read.c](https://web.eece.maine.edu/~vweaver/projects/rapl/rapl-read.c). (There was a slight change since Intel Tiger Lake is not supported by the script, Kaby Lake was used)

This results in the following output where we can see the resolution and accuracy of the power instrumentation:

```
        Detected 8 cores in 1 packages


Trying /dev/msr interface to gather results

        Listing paramaters for package #0
                Power units = 0.125W
                CPU Energy units = 0.00006104J
                DRAM Energy units = 0.00006104J
                Time units = 0.00097656s

                Package thermal spec: 35.000W
                Package minimum power: 0.000W
                Package maximum power: 0.000W
                Package maximum time window: 0.000000s
                Package power limits are unlocked
                Package power limit #1: 55.000W for 0.107422s (enabled, clamped)
                Package power limit #2: 64.000W for 0.032227s (enabled, not_clamped)
        PowerPlane1 (on-core GPU if avail) 0 policy: 16


        Sleeping 1 second

        Package 0:
                Package energy: 1.445251J
                PowerPlane0 (cores): 0.311523J
                PowerPlane1 (on-core GPU if avail): 0.112549 J
                DRAM: 0.000000J
                PSYS: 2.339600J

Note: the energy measurements can overflow in 60s or so
      so try to sample the counters more often than that.

```

### Tools for Obtaining Measurements: Using `perf`

1. **Finding Power Events:** Use the `perf list` command to find available RAPL events. This command lists all the performance monitoring events available, including those for power which can be filtered with `grep`.

2. **Measuring with `perf`:** Once identified the relevant RAPL events, using the command like `perf stat -a -e "power/energy-cores/"` to measure the energy consumption of the CPU cores while running a specific program results in the following output:

```
Performance counter stats for 'system wide':

              0.01 Joules power/energy-cores/

       0.001873665 seconds time elapsed
```

# Task 2

If we run the changes `pi_mpi.c` program and measure its busy waiting metrics using the following command:
`sudo perf stat -a -e "power/energy-cores/" mpirun --allow-run-as-root --use-hwthread-cpus pi_mpi 2000000000`

We get the following output:

```
Wall: 7.391227 seconds
Pi approximately: 3.141634

 Performance counter stats for 'system wide':

             89.52 Joules power/energy-cores/

       8.139947194 seconds time elapsed

```

If we now change the behaviour to yielding we get the following:
`sudo perf stat -a -e "power/energy-cores/" mpirun --allow-run-as-root --mca mpi_yield_when_idle 1 --use-hwthread-cpus pi_mpi 2000000000`

We get the following output:

```
Wall: 7.709884 seconds
Pi approximately: 3.141601

 Performance counter stats for 'system wide':

             79.20 Joules power/energy-cores/

       8.399686170 seconds time elapsed

```

Thus we can observe that there is now a slight difference in runtime and power consumption where the non busy waiting version is more power efficient but needs a little more time.

### Stability of Measurements

Unfortunately these measurements were not very stable, which means that there were some runs where the 2 version were almost not distinguishable from another but there is a trend visible which confirms the expected results.

### Effects of oversubscription

When oversubscription was applied these effects were greatly exagerated:
Thus using the following command which uses 16 instead of 8 ranks:
`sudo perf stat -a -e "power/energy-cores/" mpiexec --allow-run-as-root --mca mpi_yield_when_idle 1 --use-hwthread-cpus --oversubscribe -np 16 pi_mpi 2000000000`

The output is now:

```
Wall: 9.151182 seconds
Pi approximately: 3.141681

 Performance counter stats for 'system wide':

            106.32 Joules power/energy-cores/

      10.131097026 seconds time elapsed

```

While without yielding the result is:

```
Wall: 7.680165 seconds
Pi approximately: 3.141549

 Performance counter stats for 'system wide':

             99.89 Joules power/energy-cores/

       8.912444046 seconds time elapsed

```

Which shows a significant performance and power difference between the 2 versions.

# Task 3

### Setting the CPU Frequency

Using the linked Arch wiki article the Frequency of the CPU could be set using the `cpupower` tool.

To gather information the following command can be run `cpupower frequency-info` which results in:

```
analyzing CPU 6:
  driver: intel_pstate
  CPUs which run at the same hardware frequency: 6
  CPUs which need to have their frequency coordinated by software: 6
  maximum transition latency:  Cannot determine or is not supported.
  hardware limits: 400 MHz - 4.80 GHz
  available cpufreq governors: performance powersave
  current policy: frequency should be within 400 MHz and 4.80 GHz.
                  The governor "powersave" may decide which speed to use
                  within this range.
  current CPU frequency: Unable to call hardware
  current CPU frequency: 1.30 GHz (asserted by call to kernel)
  boost state support:
    Supported: yes
    Active: yes
```

To set the maximum clock frequency (clock_freq is a clock frequency with units: GHz, MHz):

`cpupower frequency-set -u clock_freq`

To set the minimum clock frequency:

`cpupower frequency-set -d clock_freq`

To set the CPU to run at a specified frequency (which unfortunately did not work):

`cpupower frequency-set -f clock_freq`

### Benchmarking

To measure the energy consumption the `perf` command from Task 1 was used.

| max Frequency | Performance (MB/s) | Power Consumption (Joules) |
| ------------- | ------------------ | -------------------------- |
| native        | 41630.8            | 6.06                       |
| 3000 MHz      | 41580.9            | 3.05                       |
| 2000 MHz      | 40955.0            | 1.79                       |
| 1000 MHz      | 32060.4            | 1.44                       |
| 400 MHz       | 16660.9            | 0.32                       |
