# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/containers-20260920/containers-n64`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| containers | deque_erase | ns/op / batch_mean | 3 | 78.552 | 73.656 | 129.038 | 214.260 | 12730340 |
| containers | deque_insert | ns/op / batch_mean | 3 | 1.694 | 1.719 | 2.347 | 2.625 | 590296993 |
| containers | deque_iterate | ns/op / batch_mean | 3 | 1.678 | 1.719 | 2.038 | 2.594 | 595958655 |
| containers | deque_lookup | ns/op / batch_mean | 3 | 45.952 | 45.703 | 53.373 | 60.171 | 21761676 |
| containers | list_erase | ns/op / batch_mean | 3 | 53.645 | 55.266 | 56.683 | 57.657 | 18641175 |
| containers | list_insert | ns/op / batch_mean | 3 | 10.445 | 10.484 | 10.661 | 11.079 | 95741021 |
| containers | list_iterate | ns/op / batch_mean | 3 | 1.919 | 1.875 | 2.349 | 2.780 | 521045347 |
| containers | list_lookup | ns/op / batch_mean | 3 | 146.307 | 146.062 | 150.067 | 158.425 | 6834934 |
| containers | map_erase | ns/op / batch_mean | 3 | 57.556 | 57.141 | 64.615 | 213.773 | 17374449 |
| containers | map_insert | ns/op / batch_mean | 3 | 27.016 | 26.781 | 31.848 | 37.835 | 37015188 |
| containers | map_iterate | ns/op / batch_mean | 3 | 6.521 | 6.422 | 7.377 | 8.908 | 153359532 |
| containers | map_lookup | ns/op / batch_mean | 3 | 6.802 | 5.953 | 8.477 | 17.363 | 147021663 |
| containers | sorted_vector_erase | ns/op / batch_mean | 3 | 35.574 | 35.531 | 38.189 | 38.342 | 28110615 |
| containers | sorted_vector_insert | ns/op / batch_mean | 3 | 40.255 | 40.219 | 41.497 | 42.611 | 24841731 |
| containers | sorted_vector_iterate | ns/op / batch_mean | 3 | 1.485 | 1.406 | 1.727 | 2.423 | 673471535 |
| containers | sorted_vector_lookup | ns/op / batch_mean | 3 | 14.072 | 14.078 | 16.142 | 17.267 | 71061368 |
| containers | unordered_map_erase | ns/op / batch_mean | 3 | 41.596 | 41.570 | 43.047 | 43.047 | 24040990 |
| containers | unordered_map_insert | ns/op / batch_mean | 3 | 32.615 | 30.844 | 34.598 | 68.962 | 30661180 |
| containers | unordered_map_iterate | ns/op / batch_mean | 3 | 4.011 | 3.922 | 4.400 | 5.235 | 249327983 |
| containers | unordered_map_lookup | ns/op / batch_mean | 3 | 5.670 | 5.641 | 6.434 | 7.127 | 176361983 |
| containers | vector_erase | ns/op / batch_mean | 3 | 27.188 | 27.078 | 29.294 | 30.393 | 36780764 |
| containers | vector_insert | ns/op / batch_mean | 3 | 1.543 | 1.562 | 1.736 | 1.861 | 647970031 |
| containers | vector_iterate | ns/op / batch_mean | 3 | 1.355 | 1.406 | 1.734 | 2.283 | 737837215 |
| containers | vector_lookup | ns/op / batch_mean | 3 | 24.974 | 17.062 | 35.236 | 35.361 | 40041042 |
| data_layout | aos | ns/op / full_pass_mean | 3 | 1.554 | 1.562 | 2.036 | 2.454 | 643410073 |
| data_layout | soa | ns/op / full_pass_mean | 3 | 2.028 | 2.031 | 2.365 | 2.908 | 493142241 |
