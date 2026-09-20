# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/phase2-toolchain-matrix/gpp-4a228b36-O1-perf_interval`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| perf_interval | dependent_chain | ns/op / batch_mean_perf_bracket | 3 | 1.301 | 1.293 | 1.392 | 1.400 | 768849398 |
