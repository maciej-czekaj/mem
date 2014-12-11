#!/bin/bash
file=${1:-res.txt}
for i in {0..20}
	do
	taskset 1 ./mem $(( (1024<<i) )) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	taskset 1 ./mem $(( ( (1024<<i) * 5) / 4 )) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	taskset 1 ./mem $(( ( (1024<<i) * 7) / 4 )) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	done | tee $file

