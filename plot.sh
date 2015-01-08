s=""
for f in $*
do
	s+="\"$f\" using (\$1*1024):(\$2) title '$f',"
done

gnuplot -e "set logscale x 2; set format x \"%.0b%B\"; set logscale y 2; plot ${s%,}; pause -1"

