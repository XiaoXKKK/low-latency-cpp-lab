# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/network-complete/payload-4096`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| network_rtt | tcp_batched | ns/sent_message / window_mean_roundtrip | 3 | 3156.931 | 3026.375 | 3789.219 | 13548.308 | 316763 |
| network_rtt | tcp_default | ns/roundtrip / successful_closed_loop_rtt | 3 | 23184.661 | 22187.500 | 30599.290 | 60399.058 | — |
| network_rtt | tcp_nodelay | ns/roundtrip / successful_closed_loop_rtt | 3 | 23816.826 | 22498.000 | 30820.700 | 206284.520 | — |
| network_rtt | tcp_unbatched | ns/sent_message / window_mean_roundtrip | 3 | 11449.497 | 11204.562 | 15955.335 | 29668.388 | 87340 |
| network_rtt | udp_batch | ns/sent_message / window_mean_roundtrip | 3 | 7314.193 | 7197.156 | 9519.882 | 18096.505 | 136720 |
| network_rtt | udp_rtt | ns/roundtrip / successful_closed_loop_rtt | 3 | 20146.564 | 19417.000 | 27151.210 | 175314.616 | — |
