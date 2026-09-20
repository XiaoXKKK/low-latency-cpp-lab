# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-4096`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 3.375 | 3.356 | 3.409 | 3.410 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.629 | 1.629 | 1.634 | 1.634 | — |
