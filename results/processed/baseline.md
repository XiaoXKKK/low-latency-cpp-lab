# Measured benchmark summary

Raw data and environment: `/home/ljw/low-latency-cpp-lab/results/raw/network-complete/baseline`

Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.
Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.

| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| network_io | blocking | ns/roundtrip / successful_closed_loop_rtt | 3 | 22804.918 | 21270.000 | 33133.800 | 97983.381 | — |
| network_io | busy_poll | ns/roundtrip / successful_closed_loop_rtt | 3 | 13768.308 | 13246.000 | 18976.100 | 33865.221 | — |
| network_io | epoll_et | ns/roundtrip / successful_closed_loop_rtt | 3 | 23412.971 | 22763.000 | 31069.300 | 64975.543 | — |
| network_io | epoll_lt | ns/roundtrip / successful_closed_loop_rtt | 3 | 23547.875 | 23034.000 | 32001.810 | 50139.446 | — |
| network_rtt | tcp_batched | ns/sent_message / window_mean_roundtrip | 3 | 1376.471 | 1337.500 | 1806.581 | 3906.721 | 726496 |
| network_rtt | tcp_default | ns/roundtrip / successful_closed_loop_rtt | 3 | 22153.793 | 21211.000 | 30127.000 | 56908.201 | — |
| network_rtt | tcp_nodelay | ns/roundtrip / successful_closed_loop_rtt | 3 | 21654.853 | 21111.000 | 29496.310 | 59323.151 | — |
| network_rtt | tcp_unbatched | ns/sent_message / window_mean_roundtrip | 3 | 8390.262 | 8200.969 | 12347.956 | 21539.695 | 119186 |
| network_rtt | udp_batch | ns/sent_message / window_mean_roundtrip | 3 | 6618.998 | 6484.875 | 8900.969 | 18528.095 | 151080 |
| network_rtt | udp_rtt | ns/roundtrip / successful_closed_loop_rtt | 3 | 17695.083 | 17163.000 | 23615.200 | 66266.471 | — |
