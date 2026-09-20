# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/locks-32`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | ns/op / batch_mean | 3 | 92.898 | 92.652 | 97.674 | 98.928 | 10764529 |
| locks | spinlock | ns/op / batch_mean | 3 | 86.436 | 85.146 | 109.741 | 112.489 | 11569241 |
| locks | ticket | ns/op / batch_mean | 3 | 131.400 | 131.461 | 136.649 | 136.934 | 7610334 |
