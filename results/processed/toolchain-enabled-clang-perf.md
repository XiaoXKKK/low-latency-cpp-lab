# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/toolchain-enabled-clang-perf`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| perf_interval | dependent_chain | ns/op / batch_mean_perf_bracket | 3 | 0.209 | 0.162 | 0.336 | 0.337 | 4781559012 |
| perf_interval | empty | ns/op / batch_mean_perf_bracket | 3 | 0.000 | 0.000 | 0.001 | 0.001 | 2055708908410 |
