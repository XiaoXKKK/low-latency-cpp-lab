# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-1-32`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 89.199 | 89.312 | 94.462 | 94.490 | 11210848 |
| locks | spinlock | batch_mean | 3 | 80.912 | 77.733 | 101.362 | 104.131 | 12359164 |
