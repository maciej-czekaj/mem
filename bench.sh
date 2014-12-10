for i in {0..19}
	do
	taskset 1 ./mem $(( (1024<<i) - 1)) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	taskset 1 ./mem $(( ( (1024<<i) * 5 - 1) / 4 )) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	taskset 1 ./mem $(( ( (1024<<i) * 7 - 1) / 4 )) | awk '{printf "%.1f %s\n", $1/1024, $2}'
	done | tee res.txt
