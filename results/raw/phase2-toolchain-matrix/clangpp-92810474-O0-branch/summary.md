# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-toolchain-matrix/clangpp-92810474-O0-branch`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| branch | random_branch | ns/op / batch_mean | 3 | 5.629 | 5.660 | 5.822 | 6.111 | 177653323 |
