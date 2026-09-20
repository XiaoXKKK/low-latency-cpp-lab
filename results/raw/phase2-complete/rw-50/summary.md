# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/rw-50`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| rw_locks | mutex | ns/op / batch_mean | 3 | 63.357 | 63.426 | 67.296 | 68.495 | 15783686 |
| rw_locks | shared_mutex | ns/op / batch_mean | 3 | 655.852 | 629.439 | 878.548 | 886.670 | 1524735 |
