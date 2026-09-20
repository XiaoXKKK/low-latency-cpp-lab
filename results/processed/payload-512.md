# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/network-complete/payload-512`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| network_rtt | tcp_batched | ns/sent_message / window_mean_roundtrip | 3 | 1545.539 | 1563.281 | 2082.163 | 3880.039 | 647023 |
| network_rtt | tcp_default | ns/roundtrip / successful_closed_loop_rtt | 3 | 21787.913 | 21361.000 | 29658.700 | 47434.766 | — |
| network_rtt | tcp_nodelay | ns/roundtrip / successful_closed_loop_rtt | 3 | 22390.116 | 21811.500 | 29852.240 | 40166.367 | — |
| network_rtt | tcp_unbatched | ns/sent_message / window_mean_roundtrip | 3 | 10016.706 | 9868.875 | 13035.469 | 22171.035 | 99833 |
| network_rtt | udp_batch | ns/sent_message / window_mean_roundtrip | 3 | 6751.182 | 6627.062 | 7633.864 | 18143.752 | 148122 |
| network_rtt | udp_rtt | ns/roundtrip / successful_closed_loop_rtt | 3 | 18110.384 | 17363.000 | 24188.300 | 61568.908 | — |
