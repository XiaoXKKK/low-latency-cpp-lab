# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/allocator-4096`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocator_batch | arena | ns/op / batch_mean | 3 | 4.973 | 4.292 | 8.483 | 8.496 | 201095321 |
| allocator_batch | malloc | ns/op / batch_mean | 3 | 16.118 | 15.812 | 17.864 | 19.381 | 62041975 |
| allocator_batch | new | ns/op / batch_mean | 3 | 18.708 | 18.444 | 21.221 | 21.835 | 53451846 |
| allocator_batch | pmr_monotonic | ns/op / batch_mean | 3 | 5.619 | 5.445 | 7.535 | 11.549 | 177957203 |
| allocator_batch | pmr_pool | ns/op / batch_mean | 3 | 24.254 | 24.050 | 26.013 | 31.844 | 41230298 |
| allocator_batch | preallocated | ns/op / batch_mean | 3 | 4.862 | 4.760 | 6.598 | 10.408 | 205674528 |
| allocator_batch | thread_local_arena | ns/op / batch_mean | 3 | 3.652 | 3.596 | 5.248 | 5.824 | 273814545 |
