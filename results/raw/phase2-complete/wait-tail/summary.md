# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/wait-tail`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| wait_strategy | hybrid | ns/op / individual_wakeup_lateness | 3 | 42035.336 | 39368.000 | 147907.340 | 649591.749 | — |
| wait_strategy | sleep | ns/op / individual_wakeup_lateness | 3 | 61022.294 | 59336.000 | 156449.920 | 725603.150 | — |
| wait_strategy | spin | ns/op / individual_wakeup_lateness | 3 | 138.715 | 52.000 | 92.000 | 11218.491 | — |
| wait_strategy | yield | ns/op / individual_wakeup_lateness | 3 | 402.354 | 293.000 | 733.010 | 8692.357 | — |
