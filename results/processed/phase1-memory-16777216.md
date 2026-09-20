# Measured benchmark summary

Raw data and environment: `results/raw/phase1-memory-16777216`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| memory_access | random | batch_mean_dependent_load | 3 | 21.430 | 21.438 | 21.461 | 21.461 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.634 | 1.635 | 1.635 | 1.635 | — |
