# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n16`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 48.943 | 43.531 | 92.907 | 112.453 | 20432140 |
| containers | deque_insert | ns/op / batch_mean | 3 | 1.829 | 1.734 | 2.205 | 2.342 | 546868324 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 4.471 | 4.375 | 5.650 | 7.878 | 223682371 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 13.011 | 13.000 | 13.173 | 14.705 | 76859335 |
| containers | list_erase | ns/op / batch_mean | 3 | 32.399 | 31.938 | 35.176 | 45.255 | 30864793 |
| containers | list_insert | ns/op / batch_mean | 3 | 21.629 | 21.609 | 22.558 | 23.484 | 46234757 |
| containers | list_iterate | ns/op / batch_mean | 3 | 2.993 | 3.125 | 5.019 | 5.569 | 334098977 |
| containers | list_lookup | ns/op / batch_mean | 3 | 7.749 | 7.672 | 8.464 | 9.439 | 129042665 |
| containers | map_erase | ns/op / batch_mean | 3 | 49.032 | 48.812 | 50.281 | 64.203 | 20394896 |
| containers | map_insert | ns/op / batch_mean | 3 | 27.132 | 26.922 | 28.777 | 39.510 | 36857018 |
| containers | map_iterate | ns/op / batch_mean | 3 | 8.861 | 8.750 | 11.975 | 15.316 | 112858856 |
| containers | map_lookup | ns/op / batch_mean | 3 | 10.332 | 10.328 | 10.664 | 12.660 | 96783462 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 6.456 | 6.250 | 6.981 | 10.823 | 154903669 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 36.810 | 36.469 | 44.096 | 216.446 | 27166877 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 2.129 | 1.875 | 2.574 | 3.632 | 469759248 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 4.829 | 4.844 | 5.477 | 6.187 | 207066132 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 43.009 | 43.188 | 45.169 | 47.941 | 23250745 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 30.842 | 30.844 | 31.794 | 32.907 | 32422957 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 6.313 | 6.250 | 8.176 | 12.686 | 158400158 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 2.802 | 2.812 | 2.978 | 3.813 | 356844159 |
| containers | vector_erase | ns/op / batch_mean | 3 | 6.237 | 6.250 | 6.362 | 10.260 | 160320641 |
| containers | vector_insert | ns/op / batch_mean | 3 | 3.203 | 3.141 | 3.609 | 3.609 | 312179894 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 4.421 | 4.375 | 5.693 | 6.194 | 226180379 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 12.399 | 10.969 | 14.262 | 134.718 | 80652275 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 4.959 | 5.000 | 6.926 | 9.685 | 201638311 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 2.386 | 2.500 | 2.581 | 4.196 | 419177364 |
