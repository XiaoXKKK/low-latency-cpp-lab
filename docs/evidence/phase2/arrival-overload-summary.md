# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/arrival-overload`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| arrival_latency | closed_idle | ns/op / individual_scheduled_response | 3 | 5490.848 | 5372.000 | 6725.000 | 11896.102 | — |
| arrival_latency | closed_idle_service | ns/op / individual_service_time | 3 | 5428.764 | 5320.000 | 6653.000 | 11731.868 | — |
| arrival_latency | open_cpu | ns/op / individual_scheduled_response | 3 | 6884446.900 | 7440870.500 | 14775449.900 | 14854316.391 | — |
| arrival_latency | open_cpu_service | ns/op / individual_service_time | 3 | 8355.367 | 5311.000 | 5461.100 | 17455.836 | — |
| arrival_latency | open_idle | ns/op / individual_scheduled_response | 3 | 5042994.525 | 5083504.000 | 9434422.900 | 9513299.380 | — |
| arrival_latency | open_idle_service | ns/op / individual_service_time | 3 | 5667.796 | 5320.000 | 10992.000 | 17734.561 | — |
| arrival_latency | open_memory | ns/op / individual_scheduled_response | 3 | 8985492.463 | 10470723.000 | 17800367.700 | 17879234.380 | — |
| arrival_latency | open_memory_service | ns/op / individual_service_time | 3 | 9857.698 | 5311.000 | 5431.000 | 3013757.611 | — |
