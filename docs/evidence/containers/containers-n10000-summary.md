# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n10000`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 4106.224 | 4068.969 | 4535.966 | 4544.054 | 243533 |
| containers | deque_insert | ns/op / batch_mean | 3 | 4.147 | 4.062 | 6.106 | 7.468 | 241127270 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 0.507 | 0.506 | 0.554 | 1.009 | 1972935274 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 2913.414 | 2884.359 | 3241.370 | 3284.068 | 343240 |
| containers | list_erase | ns/op / batch_mean | 3 | 17536.880 | 18542.938 | 22874.627 | 23323.078 | 57023 |
| containers | list_insert | ns/op / batch_mean | 3 | 12.768 | 12.219 | 16.439 | 16.578 | 78320994 |
| containers | list_iterate | ns/op / batch_mean | 3 | 1.646 | 1.630 | 2.294 | 2.495 | 607450748 |
| containers | list_lookup | ns/op / batch_mean | 3 | 10412.799 | 10423.562 | 10764.407 | 10768.611 | 96036 |
| containers | map_erase | ns/op / batch_mean | 3 | 108.253 | 107.547 | 119.820 | 124.693 | 9237635 |
| containers | map_insert | ns/op / batch_mean | 3 | 100.385 | 97.453 | 139.249 | 187.920 | 9961648 |
| containers | map_iterate | ns/op / batch_mean | 3 | 14.462 | 14.237 | 16.407 | 18.232 | 69146136 |
| containers | map_lookup | ns/op / batch_mean | 3 | 18.290 | 18.156 | 23.363 | 26.425 | 54674686 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 3263.364 | 3232.766 | 3395.162 | 3618.560 | 306432 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 3386.495 | 3362.617 | 3487.890 | 3735.616 | 295291 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 0.338 | 0.327 | 0.414 | 1.218 | 2959402911 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 17.709 | 17.688 | 18.325 | 19.453 | 56467765 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 40.951 | 40.461 | 45.159 | 51.007 | 24419652 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 28.925 | 28.812 | 32.619 | 113.485 | 34571609 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 1.661 | 1.654 | 1.739 | 2.371 | 602059284 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 3.990 | 3.922 | 4.394 | 6.064 | 250606939 |
| containers | vector_erase | ns/op / batch_mean | 3 | 2835.918 | 2812.891 | 3232.980 | 3239.537 | 352620 |
| containers | vector_insert | ns/op / batch_mean | 3 | 1.921 | 1.875 | 2.350 | 2.921 | 520536804 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 0.247 | 0.246 | 0.254 | 0.307 | 4049681493 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 1847.377 | 1829.750 | 2008.033 | 2224.838 | 541308 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.376 | 0.365 | 0.467 | 1.340 | 2661613150 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.473 | 0.472 | 0.484 | 0.544 | 2113570603 |
