# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-2-256`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 313.472 | 305.203 | 332.974 | 333.300 | 3190079 |
| locks | spinlock | batch_mean | 3 | 183.105 | 183.118 | 185.712 | 185.980 | 5461337 |
