s=""
for f in $*
do
	s+="\"$f\" using (\$1*1024):(\$2) title '$f',"
done

#draw="set term postscript enhanced color; set output '1.ps'"
draw="set term png size 1024,768; set output '1.png';"
cmd="$draw set logscale x 2; set format x \"%.0b%B\"; set logscale y 2; plot ${s%,} with points pointsize 3;"
echo $cmd
gnuplot -e "$cmd"
#set term png size 1024,768; set output '1.png'; load 'file'
