# Measured benchmark summary

Raw data and environment: `results/raw/phase1-branch-large`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| branch | random_branch | batch_mean | 3 | 3.726 | 3.708 | 3.870 | 4.139 | 268420064 |
| branch | random_branchless | batch_mean | 3 | 0.605 | 0.602 | 0.707 | 0.893 | 1653169207 |
| branch | sorted_branch | batch_mean | 3 | 0.593 | 0.647 | 0.775 | 0.779 | 1686153989 |
| branch | sorted_branchless | batch_mean | 3 | 0.604 | 0.601 | 0.702 | 0.707 | 1655870259 |
