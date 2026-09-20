# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-1-0`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 28.133 | 28.244 | 30.030 | 30.039 | 35544954 |
| locks | spinlock | batch_mean | 3 | 65.090 | 65.965 | 71.754 | 72.624 | 15363296 |
