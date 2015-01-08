s=""
for f in $*
do
	s+="\"$f\" using (\$1*1024):(\$2) title '$f',"
done

cmd="set logscale x 2; set format x \"%.0b%B\"; set logscale y 2; plot ${s%,}; pause -1"
echo $cmd
gnuplot -e "$cmd"
#set term png size 1024,768; set output '1.png'; load 'file'
