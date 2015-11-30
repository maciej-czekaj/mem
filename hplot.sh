for i in $(seq 1 100); do taskset 2 ./mem 16384 128; echo; done > out;
cut -d\  -f2 out | ./statistics.py b c
gnuplot -e "plot 'b' with impulse, 'c' ; pause -1 "
