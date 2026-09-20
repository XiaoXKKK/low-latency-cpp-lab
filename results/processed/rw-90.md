# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/rw-90`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| rw_locks | mutex | ns/op / batch_mean | 3 | 36.139 | 36.396 | 40.135 | 40.538 | 27671132 |
| rw_locks | shared_mutex | ns/op / batch_mean | 3 | 549.040 | 501.148 | 923.140 | 949.171 | 1821360 |
