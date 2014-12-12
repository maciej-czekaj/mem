s=""
for f in $*
do
	s+=\"$f\",
done

gnuplot -e "set logscale x 2; set logscale y 2; plot ${s%,}; pause -1"

