# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/rw-0`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| rw_locks | mutex | ns/op / batch_mean | 3 | 93.198 | 92.641 | 108.343 | 111.070 | 10729868 |
| rw_locks | shared_mutex | ns/op / batch_mean | 3 | 114.824 | 112.893 | 145.308 | 152.614 | 8709001 |
