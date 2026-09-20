# Measured benchmark summary

Raw data and environment: `results/raw/phase1-affinity-tail`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| affinity | pinned | individual_work_unit | 3 | 5384.743 | 5320.000 | 8428.300 | 12534.010 | — |
| affinity | unpinned | individual_work_unit | 3 | 5362.166 | 5320.000 | 5461.090 | 13115.120 | — |
