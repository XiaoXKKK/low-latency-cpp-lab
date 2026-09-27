# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/layout-gcc-n1000000`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.585 | 0.561 | 0.989 | 1.480 | 1710419365 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.491 | 0.489 | 0.513 | 0.528 | 2038599121 |
