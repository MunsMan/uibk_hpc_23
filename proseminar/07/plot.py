import matplotlib.pyplot as plt


def plot_execution_time_and_cores(
    problem_sizes,
    execution_times,
    core_numbers,
):
    for execution_time, problem_size in zip(execution_times, problem_sizes):
        plt.plot(
            core_numbers, execution_time, marker="o", label=f"#Particle={problem_size}"
        )
    plt.title("Execution Time vs. Number of Cores")
    plt.xlabel("Number of Cores")
    plt.ylabel("Execution Time (seconds)")
    plt.legend()


def plot_speedup(
    problem_sizes,
    speedups,
    core_numbers,
):
    for speedup, problem_size in zip(speedups, problem_sizes):
        plt.plot(core_numbers, speedup, marker="o", label=f"#Particle={problem_size}")
    plt.title("Speedup vs. Number of Cores")
    plt.xlabel("Number of Cores")
    plt.ylabel("Speed up")
    plt.legend()


def calculate_speedup(execution_times):
    speedup = []
    for i in range(len(execution_times)):
        baseline_execution_times = execution_times[i][0]
        speedup_row = [baseline_execution_times / time for time in execution_times[i]]
        speedup.append(speedup_row)
    return speedup


def print_speedup_table(problem_sizes, num_cores, speedup):
    # Print Markdown table header
    print(
        "| Number of Cores | Problem Size | Speedup |",
    )
    print(
        "|--------------|------------------|------------------|",
    )

    # Print table rows
    for j, core_count in enumerate(num_cores):
        row_data = [
            f"| {problem_sizes[j]} | {core_count} |"
            + " | ".join([f"{speedup[i][j]:.2f}" for i in range(len(problem_sizes))])
            + " |"
        ]
        print("".join(row_data))


if __name__ == "__main__":
    # Example data: Replace these lists with your actual data
    core_numbers = [1, 12, 24, 48, 60, 72, 96]
    problem_sizes = [10_000, 20_000]
    execution_times = [
        [14.158533, 2.310722, 1.911509, 1.792418, 1.483500, 1.572124, 2.115816],
        [19.583161, 3.939176, 3.252238, 3.199496, 3.153925, 3.067915, 3.714849],
    ]

    plot_execution_time_and_cores(problem_sizes, execution_times, core_numbers)
    plt.savefig("execution_time.png")
    plt.clf()
    # Calculate speedup
    speedup = calculate_speedup(execution_times)
    print(speedup)
    plot_speedup(problem_sizes, speedup, core_numbers)
    plt.savefig("speedup.png")
