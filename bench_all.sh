./bench.sh ${1}_random_128.txt 128

./bench.sh ${1}_sequential_8.txt 8 -l
./bench.sh ${1}_sequential_64.txt 64 -l
#./bench.sh ${1}_sequential_128.txt 128 -l
./bench.sh ${1}_sequential_256.txt 256 -l

#files="\"${1}_random_128.txt\",\"${1}_sequential_8.txt\",\"${1}_sequential_64.txt\",\"${1}_sequential_128.txt\",\"${1}_sequential_256.txt\""
files="\"${1}_random_128.txt\",\"${1}_sequential_8.txt\",\"${1}_sequential_64.txt\",,\"${1}_sequential_256.txt\""

echo "set logscale x 2; set logscale y 2; plot $files; pause -1" > ${1}.plot

