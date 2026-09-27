# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n100000`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 57155.560 | 56796.922 | 65436.156 | 68631.212 | 17496 |
| containers | deque_insert | ns/op / batch_mean | 3 | 5.803 | 5.711 | 8.623 | 9.813 | 172339509 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 0.907 | 0.897 | 0.986 | 1.163 | 1102676980 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 50043.871 | 49878.828 | 53944.184 | 55024.229 | 19982 |
| containers | list_erase | ns/op / batch_mean | 3 | 154092.345 | 152301.039 | 210051.815 | 238065.621 | 6490 |
| containers | list_insert | ns/op / batch_mean | 3 | 12.691 | 12.211 | 16.036 | 22.036 | 78796385 |
| containers | list_iterate | ns/op / batch_mean | 3 | 1.668 | 1.648 | 1.790 | 1.940 | 599429104 |
| containers | list_lookup | ns/op / batch_mean | 3 | 106341.423 | 105942.719 | 111314.758 | 115127.672 | 9404 |
| containers | map_erase | ns/op / batch_mean | 3 | 235.578 | 230.828 | 287.311 | 374.908 | 4244885 |
| containers | map_insert | ns/op / batch_mean | 3 | 228.712 | 223.234 | 258.071 | 265.755 | 4372319 |
| containers | map_iterate | ns/op / batch_mean | 3 | 22.679 | 22.605 | 23.913 | 25.385 | 44092686 |
| containers | map_lookup | ns/op / batch_mean | 3 | 32.634 | 32.562 | 34.483 | 38.534 | 30642536 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 32111.950 | 32018.445 | 34657.542 | 40331.174 | 31141 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 35319.637 | 35078.609 | 41269.824 | 45439.064 | 28313 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 0.369 | 0.361 | 0.440 | 0.475 | 2709101307 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 21.903 | 21.922 | 22.403 | 23.517 | 45655586 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 74.582 | 74.125 | 88.888 | 113.711 | 13408111 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 46.495 | 46.188 | 56.545 | 59.205 | 21507472 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 1.640 | 1.624 | 1.725 | 1.886 | 609630975 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 3.906 | 3.906 | 4.097 | 5.330 | 256020482 |
| containers | vector_erase | ns/op / batch_mean | 3 | 34245.380 | 34243.250 | 36258.250 | 39420.272 | 29201 |
| containers | vector_insert | ns/op / batch_mean | 3 | 1.971 | 1.875 | 2.970 | 3.124 | 507412987 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 0.307 | 0.290 | 0.419 | 0.467 | 3256777354 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 23438.627 | 23336.898 | 26281.430 | 29789.494 | 42665 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 0.464 | 0.445 | 0.568 | 0.615 | 2155748984 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 0.494 | 0.487 | 0.571 | 0.740 | 2024925209 |
