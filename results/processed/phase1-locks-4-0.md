# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-4-0`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 4.156 | 3.706 | 6.527 | 6.535 | 240612245 |
| locks | spinlock | batch_mean | 3 | 3.170 | 3.011 | 4.739 | 4.902 | 315472477 |
