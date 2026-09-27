# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n1024`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 395.348 | 391.531 | 468.050 | 521.203 | 2529415 |
| containers | deque_insert | ns/op / batch_mean | 3 | 2.715 | 2.516 | 5.961 | 6.563 | 368324125 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 0.856 | 0.852 | 0.874 | 1.180 | 1168056395 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 354.998 | 284.766 | 595.915 | 617.535 | 2816920 |
| containers | list_erase | ns/op / batch_mean | 3 | 1115.817 | 1115.094 | 1250.377 | 1488.608 | 896205 |
| containers | list_insert | ns/op / batch_mean | 3 | 11.601 | 10.500 | 23.016 | 24.345 | 86197608 |
| containers | list_iterate | ns/op / batch_mean | 3 | 1.724 | 1.703 | 2.098 | 3.309 | 580045089 |
| containers | list_lookup | ns/op / batch_mean | 3 | 1030.291 | 1018.273 | 1304.732 | 1336.544 | 970600 |
| containers | map_erase | ns/op / batch_mean | 3 | 37.820 | 36.312 | 62.353 | 136.988 | 26440818 |
| containers | map_insert | ns/op / batch_mean | 3 | 33.822 | 33.812 | 34.920 | 35.842 | 29566254 |
| containers | map_iterate | ns/op / batch_mean | 3 | 5.815 | 5.714 | 6.536 | 14.356 | 171962115 |
| containers | map_lookup | ns/op / batch_mean | 3 | 23.140 | 23.016 | 24.788 | 29.535 | 43214628 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 398.351 | 390.594 | 529.419 | 853.627 | 2510348 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 405.601 | 387.469 | 813.244 | 823.838 | 2465478 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 0.730 | 0.725 | 0.784 | 0.862 | 1370376318 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 14.438 | 12.844 | 17.020 | 31.736 | 69259572 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 27.022 | 25.672 | 30.695 | 54.407 | 37007054 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 18.213 | 18.156 | 19.881 | 24.239 | 54905460 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 3.442 | 3.435 | 3.543 | 3.612 | 290492333 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 3.713 | 3.750 | 3.925 | 4.455 | 269292266 |
| containers | vector_erase | ns/op / batch_mean | 3 | 290.366 | 286.016 | 372.714 | 453.124 | 3443928 |
| containers | vector_insert | ns/op / batch_mean | 3 | 1.595 | 1.562 | 1.898 | 2.580 | 627020672 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 0.557 | 0.558 | 0.627 | 0.688 | 1794319158 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 264.729 | 187.547 | 419.306 | 535.637 | 3777454 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.438 | 0.440 | 0.442 | 0.555 | 2284388525 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 1.043 | 1.037 | 1.126 | 1.196 | 958855366 |
