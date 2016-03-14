bench=${BENCH:-./thr_bench}

./l1.sh $bench 16 a
./l1.sh $bench 128 a

./l1.sh $bench 16 s
./l1.sh $bench 128 s

./l1.sh $bench 16 r
./l1.sh $bench 128 r

./l1.sh $bench 16 w
./l1.sh $bench 128 w
