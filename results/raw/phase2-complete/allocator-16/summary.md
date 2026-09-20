# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/allocator-16`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocator_batch | arena | ns/op / batch_mean | 3 | 10.803 | 10.625 | 11.369 | 16.381 | 92565809 |
| allocator_batch | malloc | ns/op / batch_mean | 3 | 31.691 | 31.594 | 33.888 | 40.570 | 31555073 |
| allocator_batch | new | ns/op / batch_mean | 3 | 36.483 | 36.312 | 38.281 | 46.634 | 27409933 |
| allocator_batch | pmr_monotonic | ns/op / batch_mean | 3 | 14.711 | 14.438 | 15.131 | 19.066 | 67975189 |
| allocator_batch | pmr_pool | ns/op / batch_mean | 3 | 53.717 | 53.812 | 53.950 | 60.633 | 18616124 |
| allocator_batch | preallocated | ns/op / batch_mean | 3 | 10.796 | 10.625 | 11.343 | 14.072 | 92630116 |
| allocator_batch | thread_local_arena | ns/op / batch_mean | 3 | 4.709 | 4.438 | 5.100 | 8.441 | 212370587 |
