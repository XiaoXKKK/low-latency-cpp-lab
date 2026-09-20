# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-32768`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 2.979 | 2.886 | 3.172 | 3.177 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 3.362 | 3.362 | 3.371 | 3.371 | — |
