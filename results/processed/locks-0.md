# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/locks-0`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| locks | mutex | ns/op / batch_mean | 3 | 28.326 | 28.587 | 30.712 | 31.018 | 35303564 |
| locks | spinlock | ns/op / batch_mean | 3 | 79.169 | 81.452 | 85.688 | 86.366 | 12631190 |
| locks | ticket | ns/op / batch_mean | 3 | 106.219 | 106.470 | 116.214 | 120.679 | 9414515 |
