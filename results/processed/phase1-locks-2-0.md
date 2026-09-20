# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-2-0`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 15.512 | 15.556 | 17.871 | 17.972 | 64468079 |
| locks | spinlock | batch_mean | 3 | 14.000 | 15.737 | 20.496 | 21.097 | 71430290 |
