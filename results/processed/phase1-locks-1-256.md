# Measured benchmark summary

Raw data and environment: `results/raw/phase1-locks-1-256`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | batch_mean | 3 | 533.886 | 529.693 | 598.310 | 607.351 | 1873058 |
| locks | spinlock | batch_mean | 3 | 390.864 | 387.653 | 437.059 | 447.548 | 2558434 |
