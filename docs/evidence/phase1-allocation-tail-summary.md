# Measured benchmark summary

Raw data and environment: `results/raw/phase1-allocation-tail`

Values below are medians across independent rounds; all time values are ns in the documented sample unit.
Batch percentiles describe batch means, NOT individual-operation tails. Small samples cannot establish p99.9.

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocation | malloc_free | individual_allocate_touch_free | 3 | 41.345 | 40.000 | 81.000 | 81.000 | — |
| allocation | new_delete | individual_allocate_touch_free | 3 | 43.751 | 40.000 | 81.000 | 81.000 | — |
| allocation | pool | individual_allocate_touch_free | 3 | 37.619 | 30.000 | 71.000 | 71.000 | — |
| allocation | preallocated | individual_allocate_touch_free | 3 | 35.716 | 30.000 | 70.000 | 71.000 | — |
