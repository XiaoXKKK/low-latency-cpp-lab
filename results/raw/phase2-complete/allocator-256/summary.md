# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/allocator-256`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocator_batch | arena | ns/op / batch_mean | 3 | 4.719 | 4.656 | 5.989 | 6.556 | 211924038 |
| allocator_batch | malloc | ns/op / batch_mean | 3 | 31.760 | 30.939 | 33.040 | 81.222 | 31485836 |
| allocator_batch | new | ns/op / batch_mean | 3 | 17.911 | 17.496 | 20.988 | 49.827 | 55830589 |
| allocator_batch | pmr_monotonic | ns/op / batch_mean | 3 | 11.348 | 11.312 | 11.860 | 11.895 | 88121194 |
| allocator_batch | pmr_pool | ns/op / batch_mean | 3 | 31.348 | 30.844 | 34.195 | 56.215 | 31900391 |
| allocator_batch | preallocated | ns/op / batch_mean | 3 | 8.173 | 7.828 | 10.336 | 42.955 | 122359239 |
| allocator_batch | thread_local_arena | ns/op / batch_mean | 3 | 7.984 | 7.943 | 10.449 | 10.449 | 125253197 |
