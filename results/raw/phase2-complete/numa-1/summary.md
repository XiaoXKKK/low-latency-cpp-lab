# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/numa-1`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| numa_access | bound_random | ns/op / batch_mean_dependent_load | 3 | 175.586 | 172.932 | 182.390 | 182.563 | — |
| numa_access | bound_streaming | ns/op / batch_mean_streaming_load | 3 | 0.429 | 0.425 | 0.435 | 0.435 | 2331438551 |
| numa_access | first_touch_streaming | ns/op / batch_mean_streaming_load | 3 | 0.489 | 0.473 | 0.532 | 0.534 | 2046101428 |

## NOT MEASURED

- numa_access/first_touch_random: Actual pages not all on requested node; locality comparison rejected
