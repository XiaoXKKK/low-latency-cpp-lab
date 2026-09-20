# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/rw-100`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| rw_locks | mutex | ns/op / batch_mean | 3 | 26.199 | 26.285 | 28.640 | 28.706 | 38169372 |
| rw_locks | shared_mutex | ns/op / batch_mean | 3 | 56.204 | 56.183 | 66.767 | 68.220 | 17792210 |
