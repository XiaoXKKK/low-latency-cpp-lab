# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/numa-0`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| numa_access | bound_random | ns/op / batch_mean_dependent_load | 3 | 77.035 | 77.451 | 78.630 | 78.651 | — |
| numa_access | bound_streaming | ns/op / batch_mean_streaming_load | 3 | 0.199 | 0.199 | 0.200 | 0.200 | 5030670594 |
| numa_access | first_touch_random | ns/op / batch_mean_dependent_load | 3 | 77.473 | 77.391 | 77.740 | 77.746 | — |
| numa_access | first_touch_streaming | ns/op / batch_mean_streaming_load | 3 | 0.202 | 0.202 | 0.202 | 0.202 | 4950789602 |
