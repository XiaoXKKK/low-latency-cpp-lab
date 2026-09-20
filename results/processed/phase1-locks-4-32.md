# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-4-32`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 12.181 | 12.099 | 14.032 | 14.931 | 82097445 |
| locks | spinlock | batch_mean | 3 | 12.232 | 12.054 | 14.329 | 14.493 | 81752032 |
