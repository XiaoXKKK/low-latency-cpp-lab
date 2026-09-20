# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/network-complete/same-cpu`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| network_io | blocking | ns/roundtrip / successful_closed_loop_rtt | 3 | 24080.243 | 23265.000 | 33826.200 | 52196.241 | — |
| network_io | busy_poll | ns/roundtrip / successful_closed_loop_rtt | 3 | 6012689.921 | 5999799.000 | 7000684.000 | 8997290.691 | — |
| network_io | epoll_et | ns/roundtrip / successful_closed_loop_rtt | 3 | 26299.154 | 25990.000 | 37571.110 | 54170.622 | — |
| network_io | epoll_lt | ns/roundtrip / successful_closed_loop_rtt | 3 | 27336.067 | 27132.000 | 67768.120 | 150901.491 | — |
