# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/locks-oversubscribed`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | ns/op / batch_mean | 3 | 540.263 | 530.703 | 583.578 | 593.502 | 1850952 |
| locks | spinlock | ns/op / batch_mean | 3 | 511.631 | 511.125 | 518.900 | 519.074 | 1954533 |
| locks | ticket | ns/op / batch_mean | 3 | 256.712 | 204.438 | 648.055 | 683.612 | 3895408 |
