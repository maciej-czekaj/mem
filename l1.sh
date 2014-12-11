perf stat -e cache-references,cache-misses,L1-dcache-load-misses,L1-dcache-prefetch-misses,L1-dcache-loads,LLC-loads,LLC-prefetches,dTLB-loads,dTLB-load-misses,page-faults taskset 1 $@
