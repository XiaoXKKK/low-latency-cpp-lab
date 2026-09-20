# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-4194304`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 16.722 | 16.719 | 16.744 | 16.745 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.633 | 1.635 | 1.636 | 1.636 | — |
