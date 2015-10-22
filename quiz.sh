#./quiz | tee quiz.txt
draw="set term png size 1024,768; set output 'quiz.png';"
cmd="$draw plot  [0:16][0:180] 'quiz.txt' title 'quiz' with linespoints pointsize 3"
gnuplot -e "$cmd"
