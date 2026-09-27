# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/layout-clang-n1000000`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.604 | 0.593 | 0.885 | 1.215 | 1656807996 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.349 | 0.345 | 0.364 | 0.435 | 2867262574 |
