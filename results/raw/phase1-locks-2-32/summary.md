# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-2-32`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 52.283 | 52.311 | 59.854 | 60.863 | 19126782 |
| locks | spinlock | batch_mean | 3 | 37.023 | 40.954 | 45.922 | 47.924 | 27010030 |
