# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n256`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 137.767 | 132.438 | 155.540 | 441.760 | 7258639 |
| containers | deque_insert | ns/op / batch_mean | 3 | 2.483 | 2.656 | 3.125 | 3.125 | 402794386 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 0.524 | 0.512 | 0.553 | 0.723 | 1908596138 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 80.193 | 79.219 | 81.144 | 157.030 | 12469970 |
| containers | list_erase | ns/op / batch_mean | 3 | 234.388 | 187.078 | 402.559 | 409.236 | 4266433 |
| containers | list_insert | ns/op / batch_mean | 3 | 16.216 | 19.719 | 21.769 | 22.047 | 61668915 |
| containers | list_iterate | ns/op / batch_mean | 3 | 3.506 | 3.484 | 3.568 | 4.020 | 285259017 |
| containers | list_lookup | ns/op / batch_mean | 3 | 297.663 | 258.625 | 459.432 | 567.549 | 3359508 |
| containers | map_erase | ns/op / batch_mean | 3 | 39.449 | 33.188 | 69.998 | 71.251 | 25349446 |
| containers | map_insert | ns/op / batch_mean | 3 | 48.480 | 35.531 | 70.316 | 75.927 | 20627262 |
| containers | map_iterate | ns/op / batch_mean | 3 | 3.850 | 3.836 | 4.862 | 5.485 | 259748166 |
| containers | map_lookup | ns/op / batch_mean | 3 | 17.992 | 18.000 | 19.280 | 21.939 | 55580161 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 134.578 | 96.500 | 207.197 | 399.577 | 7430628 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 122.045 | 121.867 | 133.081 | 133.346 | 8193720 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 0.441 | 0.430 | 0.473 | 0.540 | 2265085826 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 17.861 | 20.039 | 22.108 | 24.767 | 55986633 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 42.302 | 44.922 | 47.422 | 47.436 | 23639614 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 35.624 | 30.844 | 40.716 | 41.829 | 28071284 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 3.512 | 3.520 | 3.644 | 3.957 | 284713340 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 7.395 | 7.359 | 7.537 | 9.487 | 135226504 |
| containers | vector_erase | ns/op / batch_mean | 3 | 151.441 | 159.219 | 188.649 | 342.499 | 6603214 |
| containers | vector_insert | ns/op / batch_mean | 3 | 2.732 | 2.656 | 3.142 | 3.281 | 366069896 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 0.347 | 0.352 | 0.357 | 0.458 | 2884182064 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 58.489 | 54.797 | 105.217 | 137.121 | 17097259 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.847 | 0.859 | 0.866 | 1.141 | 1180321822 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.604 | 0.590 | 0.630 | 0.770 | 1654815772 |
