# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-262144`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 4.097 | 4.077 | 4.442 | 4.452 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.681 | 1.625 | 1.801 | 1.804 | — |
