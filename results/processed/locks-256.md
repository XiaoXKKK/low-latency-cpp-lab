# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/locks-256`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | ns/op / batch_mean | 3 | 534.530 | 528.999 | 579.070 | 581.084 | 1870804 |
| locks | spinlock | ns/op / batch_mean | 3 | 375.493 | 372.429 | 401.854 | 401.950 | 2663165 |
| locks | ticket | ns/op / batch_mean | 3 | 427.034 | 424.454 | 461.956 | 471.834 | 2341734 |
