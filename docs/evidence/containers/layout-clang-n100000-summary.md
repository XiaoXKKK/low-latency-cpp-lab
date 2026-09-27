# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/layout-clang-n100000`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.488 | 0.479 | 0.554 | 0.601 | 2050170123 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.342 | 0.340 | 0.405 | 0.421 | 2926693641 |
