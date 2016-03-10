#!/bin/bash
file=${1:-res.txt}

stride=${2:-128}

bench=${BENCH:-./mem_bench}

function run {
	$bench $1 $2 $3 $4 | awk '{printf "%.1f %s\n", $1/1024, $2}'
}

for i in {0..18}
	do
	run $(( (1024<<i) )) $stride $3 $4
	run $(( ( (1024<<i) * 5) / 4 )) $stride $3 $4
	run $(( ( (1024<<i) * 7) / 4 )) $stride $3 $4
	done | stdbuf -oL tee $file

