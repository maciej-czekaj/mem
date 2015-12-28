for i in $(seq 1 10); do taskset 2 ./mem $((1024*1024)) 128; echo; done > out
cut -d\  -f2 out > samples.txt
cat samples.txt | ./statistics.py b c
gnuplot -e "plot 'b' with impulse, 'c' ; pause -1 "
