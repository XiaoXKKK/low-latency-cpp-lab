# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/main`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocation_handoff | cross_thread_free | ns/op / batch_mean | 3 | 35.459 | 35.312 | 43.405 | 49.616 | 28201982 |
| allocation_handoff | same_thread_free | ns/op / batch_mean | 3 | 28.776 | 27.944 | 33.774 | 34.084 | 34751534 |
| allocator_batch | arena | ns/op / batch_mean | 3 | 5.442 | 4.325 | 9.101 | 9.908 | 183763161 |
| allocator_batch | malloc | ns/op / batch_mean | 3 | 18.244 | 15.635 | 32.857 | 39.051 | 54812295 |
| allocator_batch | new | ns/op / batch_mean | 3 | 20.915 | 18.052 | 38.300 | 43.488 | 47812262 |
| allocator_batch | pmr_monotonic | ns/op / batch_mean | 3 | 5.547 | 5.504 | 5.957 | 8.900 | 180276295 |
| allocator_batch | pmr_pool | ns/op / batch_mean | 3 | 24.947 | 24.167 | 49.767 | 50.032 | 40085526 |
| allocator_batch | preallocated | ns/op / batch_mean | 3 | 7.456 | 9.016 | 10.153 | 16.305 | 134115411 |
| allocator_batch | thread_local_arena | ns/op / batch_mean | 3 | 7.342 | 7.490 | 11.934 | 13.990 | 136194210 |
| arrival_latency | closed_idle | ns/op / individual_scheduled_response | 3 | 3284.080 | 2727.000 | 8705.410 | 9276.541 | — |
| arrival_latency | closed_idle_service | ns/op / individual_service_time | 3 | 3216.280 | 2665.000 | 8653.100 | 9196.610 | — |
| arrival_latency | open_cpu | ns/op / individual_scheduled_response | 3 | 735346.250 | 2743.500 | 2950714.270 | 2952074.827 | — |
| arrival_latency | open_cpu_service | ns/op / individual_service_time | 3 | 2672.120 | 2665.000 | 2805.200 | 2823.020 | — |
| arrival_latency | open_idle | ns/op / individual_scheduled_response | 3 | 2885.310 | 2736.000 | 3436.270 | 3460.327 | — |
| arrival_latency | open_idle_service | ns/op / individual_service_time | 3 | 2818.010 | 2665.000 | 3357.000 | 3357.000 | — |
| arrival_latency | open_memory | ns/op / individual_scheduled_response | 3 | 744039.970 | 2748.000 | 2944439.570 | 2977991.957 | — |
| arrival_latency | open_memory_service | ns/op / individual_service_time | 3 | 2677.950 | 2665.000 | 2915.810 | 2987.981 | — |
| atomic_order | acq_rel_rmw | ns/op / batch_mean | 3 | 5.163 | 5.090 | 6.250 | 12.267 | 193704051 |
| atomic_order | acquire_rmw | ns/op / batch_mean | 3 | 5.353 | 5.308 | 10.492 | 11.768 | 186797912 |
| atomic_order | relaxed_rmw | ns/op / batch_mean | 3 | 5.090 | 5.027 | 5.606 | 9.014 | 196465916 |
| atomic_order | relaxed_store | ns/op / batch_mean | 3 | 0.828 | 0.827 | 0.852 | 0.856 | 1207276672 |
| atomic_order | release_rmw | ns/op / batch_mean | 3 | 5.076 | 5.020 | 5.744 | 8.777 | 196995624 |
| atomic_order | release_store | ns/op / batch_mean | 3 | 1.232 | 1.238 | 1.238 | 1.242 | 811577663 |
| atomic_order | seq_cst_rmw | ns/op / batch_mean | 3 | 5.077 | 5.024 | 5.683 | 10.149 | 196956408 |
| atomic_order | seq_cst_store | ns/op / batch_mean | 3 | 3.261 | 3.253 | 3.317 | 12.863 | 306672217 |
| cache_patterns | indexed | ns/op / batch_mean | 3 | 0.959 | 0.954 | 1.004 | 1.294 | 1042325798 |
| cache_patterns | prefetch | ns/op / batch_mean | 3 | 1.029 | 1.021 | 1.132 | 1.286 | 971956026 |
| cache_patterns | streaming | ns/op / batch_mean | 3 | 0.185 | 0.180 | 0.253 | 0.382 | 5391260980 |
| cache_patterns | stride | ns/op / batch_mean | 3 | 0.180 | 0.178 | 0.205 | 0.233 | 5559988377 |
| locks | mutex | ns/op / batch_mean | 3 | 26.414 | 27.481 | 32.134 | 32.553 | 37858870 |
| locks | spinlock | ns/op / batch_mean | 3 | 21.636 | 19.339 | 39.508 | 41.823 | 46219319 |
| locks | ticket | ns/op / batch_mean | 3 | 81.210 | 82.267 | 91.374 | 92.244 | 12313730 |
| numa_access | bound_random | ns/op / batch_mean_dependent_load | 3 | 18.557 | 17.739 | 31.479 | 33.659 | — |
| numa_access | bound_streaming | ns/op / batch_mean_streaming_load | 3 | 0.117 | 0.115 | 0.129 | 0.138 | 8546011865 |
| numa_access | first_touch_random | ns/op / batch_mean_dependent_load | 3 | 18.490 | 17.739 | 31.339 | 32.261 | — |
| numa_access | first_touch_streaming | ns/op / batch_mean_streaming_load | 3 | 0.116 | 0.116 | 0.129 | 0.131 | 8583929830 |
| page_behavior | first_touch | ns/op / batch_mean_page_touch | 3 | 1844.906 | 1829.159 | 2207.723 | 2222.160 | — |
| page_behavior | mapping_roundtrip | ns/op / individual_guarded_mmap_mprotect_munmap | 3 | 14903.780 | 17203.000 | 20601.050 | 31608.174 | — |
| page_behavior | thp | ns/op / batch_mean_page_touch | 2 | 5.346 | 4.530 | 7.612 | 7.700 | — |
| page_behavior | warm_touch | ns/op / batch_mean_page_touch | 3 | 3.723 | 3.697 | 4.188 | 4.207 | — |
| rw_locks | mutex | ns/op / batch_mean | 3 | 27.824 | 28.788 | 34.355 | 67.674 | 35940127 |
| rw_locks | shared_mutex | ns/op / batch_mean | 3 | 127.844 | 104.669 | 278.365 | 292.626 | 7822003 |
| tsc_interval | dependent_chain | tsc_ticks/op / batch_mean_serialized_tsc | 3 | 5.504 | 5.488 | 6.076 | 6.233 | — |
| tsc_interval | empty | tsc_ticks/op / batch_mean_serialized_tsc | 3 | 0.099 | 0.088 | 0.106 | 0.837 | — |
| wait_strategy | hybrid | ns/op / individual_wakeup_lateness | 3 | 39715.580 | 35430.000 | 146323.720 | 170242.440 | — |
| wait_strategy | sleep | ns/op / individual_wakeup_lateness | 3 | 54442.690 | 54677.000 | 57490.670 | 116267.267 | — |
| wait_strategy | spin | ns/op / individual_wakeup_lateness | 3 | 60.890 | 53.000 | 102.400 | 138.040 | — |
| wait_strategy | yield | ns/op / individual_wakeup_lateness | 3 | 316.970 | 303.000 | 747.200 | 923.110 | — |

## NOT MEASURED

- page_behavior/hugetlb: MAP_HUGETLB 2MiB: Cannot allocate memory
- page_behavior/thp: madvise: Cannot allocate memory
- perf_interval/dependent_chain: perf_event_open cycles: Permission denied
- perf_interval/empty: perf_event_open cycles: Permission denied
