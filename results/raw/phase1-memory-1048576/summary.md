# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-1048576`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 12.435 | 12.467 | 12.508 | 12.509 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.633 | 1.625 | 1.651 | 1.652 | — |
