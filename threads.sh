bench=${BENCH:-./thr_bench}

B='a s r w'

for b in $B
do
	b0=`$bench 0 $b`
	b1=`$bench 16 $b`
	b2=`$bench 64 $b`
	all="$all$b $b0\n$b $b1\n$b $b2\n"
done

echo -en $all
