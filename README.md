# IPC Test Environment
This project acts as a testing bed for various inter-process communication (IPC) techniques. This setup benchmarks the following IPC techniques:
 - Shared Memory
 - Pipes
 - Messages Queue

## Building
Build the project using meson and ninja, by executing `./configure`.

## Run the Experiments
Run the experiments by configuring, and running `./run_test`

## Results
Results are placed in three different files:
 - `time_experiment.txt` performance stats (latency and throughput).
 - `perf_experiment.txt` perf stats (cache-misses,cache-references,context-switches,cpu-cycles,instructions).
 - `top_experiment.txt` resource stats (%MEM, %CPU and total memory used).