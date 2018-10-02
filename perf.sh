#!/usr/bin/bash
perf stat -e L1-dcache-prefetch-misses,L1-dcache-prefetches,L1-dcache-load-misses,LLC-load-misses ./test.run
