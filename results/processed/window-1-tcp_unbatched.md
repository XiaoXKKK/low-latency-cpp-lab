# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/network-complete/window-1-tcp_unbatched`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| network_rtt | tcp_unbatched | ns/sent_message / window_mean_roundtrip | 3 | 20998.928 | 20670.000 | 28455.810 | 43108.191 | 47621 |
