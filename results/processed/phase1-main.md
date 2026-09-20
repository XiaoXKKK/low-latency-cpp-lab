# Measured benchmark summary

Raw data and environment: `results/raw/phase1-main`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| affinity | pinned | individual_work_unit | 3 | 5985.945 | 5331.000 | 6690.610 | 13096.845 | — |
| affinity | unpinned | individual_work_unit | 3 | 5365.145 | 5320.000 | 5354.610 | 12391.215 | — |
| allocation | malloc_free | individual_allocate_touch_free | 3 | 46.740 | 50.000 | 51.000 | 83.040 | — |
| allocation | new_delete | individual_allocate_touch_free | 3 | 48.350 | 50.000 | 51.000 | 82.239 | — |
| allocation | pool | individual_allocate_touch_free | 3 | 41.625 | 40.000 | 50.000 | 74.030 | — |
| allocation | preallocated | individual_allocate_touch_free | 3 | 31.770 | 30.000 | 41.000 | 48.209 | — |
| branch | random_branch | batch_mean | 3 | 1.023 | 0.974 | 1.235 | 2.507 | 977739479 |
| branch | random_branchless | batch_mean | 3 | 0.657 | 0.614 | 0.768 | 0.925 | 1520960552 |
| branch | sorted_branch | batch_mean | 3 | 0.620 | 0.619 | 0.621 | 0.645 | 1614126762 |
| branch | sorted_branchless | batch_mean | 3 | 0.777 | 0.766 | 0.771 | 3.080 | 1287188141 |
| false_sharing | packed | batch_mean | 3 | 14.549 | 16.008 | 17.698 | 18.096 | 68732103 |
| false_sharing | padded | batch_mean | 3 | 2.892 | 2.741 | 3.335 | 6.077 | 345796120 |
| locks | mutex | batch_mean | 3 | 26.819 | 26.943 | 29.938 | 38.776 | 37286844 |
| locks | spinlock | batch_mean | 3 | 27.557 | 27.950 | 34.303 | 35.062 | 36288519 |
| memory_access | random | batch_mean_dependent_load | 3 | 1.888 | 1.673 | 3.503 | 4.310 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.750 | 1.624 | 2.478 | 2.979 | — |
| spsc | naive | batch_mean | 3 | 22.392 | 22.887 | 26.038 | 29.343 | 44659789 |
| spsc | naive_latency | individual_message_instrumented | 3 | 9444.681 | 9398.000 | 24697.000 | 38744.000 | — |
| spsc | padded | batch_mean | 3 | 20.262 | 20.267 | 23.037 | 50.180 | 49352837 |
| spsc | padded_latency | individual_message_instrumented | 3 | 62163.514 | 70215.000 | 92978.000 | 95763.000 | — |
| timer | clock_gettime | batch_mean | 3 | 29.004 | 28.800 | 30.530 | 35.280 | 34477551 |
| timer | empty_loop | batch_mean | 3 | 0.339 | 0.333 | 0.340 | 0.793 | 2948831015 |
| timer | lfence_rdtsc | batch_mean | 3 | 27.703 | 27.484 | 29.361 | 34.803 | 36097571 |
| timer | rdtsc_raw | batch_mean | 3 | 11.896 | 11.643 | 14.591 | 15.701 | 84064813 |
| timer | rdtscp_lfence | batch_mean | 3 | 29.956 | 29.744 | 31.700 | 33.961 | 33382550 |
| timer | steady_clock | batch_mean | 3 | 30.676 | 30.390 | 34.578 | 39.965 | 32599239 |
