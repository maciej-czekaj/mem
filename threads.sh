bench=${BENCH:-./thr_bench}

a1=`$bench 16 a`
a2=`$bench 128 a`

s1=`$bench 16 s`
s2=`$bench 128 s`

r1=`$bench 16 r`
r2=`$bench 128 r`

w1=`$bench 16 w`
w2=`$bench 128 w`

echo $a1 $a2
echo $s1 $s2
echo $r1 $r2
echo $w1 $w2
