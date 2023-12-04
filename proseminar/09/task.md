# Task 1

Laptop CPU: `11th Gen Intel(R) Core(TM) i7-11370H @ 3.30GHz`

The Intel Core i7-11370H supports basic Intel RAPL features. This includes monitoring the power consumption of the CPU cores and the whole CPU Package.

### Tools for Obtaining Measurements: Using `perf`

1. **Finding Power Events:** Use the `perf list` command to find available RAPL events. This command lists all the performance monitoring events available, including those for power which can be filtered with `grep`.

2. **Measuring with `perf`:** Once identified the relevant RAPL events, using the command like `perf stat -a -e "power/energy-cores/"` to measure the energy consumption of the CPU cores while running a specific program results in the following output:

```
Performance counter stats for 'system wide':

              0.01 Joules power/energy-cores/

       0.001873665 seconds time elapsed
```

# Task 2

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
