# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-complete/arrival-tail`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| arrival_latency | closed_idle | ns/op / individual_scheduled_response | 3 | 5516.855 | 5389.000 | 11099.000 | 19015.681 | — |
| arrival_latency | closed_idle_service | ns/op / individual_service_time | 3 | 5411.595 | 5311.000 | 10851.000 | 12574.150 | — |
| arrival_latency | open_cpu | ns/op / individual_scheduled_response | 3 | 1089062.536 | 826045.000 | 4541795.200 | 5857113.609 | — |
| arrival_latency | open_cpu_service | ns/op / individual_service_time | 3 | 5347.161 | 5320.000 | 5660.000 | 12985.060 | — |
| arrival_latency | open_idle | ns/op / individual_scheduled_response | 3 | 5512.690 | 5379.000 | 5586.400 | 24910.047 | — |
| arrival_latency | open_idle_service | ns/op / individual_service_time | 3 | 5324.515 | 5311.000 | 5350.000 | 10020.951 | — |
| arrival_latency | open_memory | ns/op / individual_scheduled_response | 3 | 1031623.789 | 804689.000 | 2977869.650 | 3010000.328 | — |
| arrival_latency | open_memory_service | ns/op / individual_service_time | 3 | 5339.982 | 5320.000 | 5591.100 | 11943.120 | — |
