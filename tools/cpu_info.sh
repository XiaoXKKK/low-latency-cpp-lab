#!/usr/bin/env bash
set -eu
lscpu
numactl --hardware || true
