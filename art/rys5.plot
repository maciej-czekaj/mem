set term png size 640,480; set output 'Rys5.png'; set logscale x 2; set format x "%.0b%B"; set logscale y 2; 
plot "../corei7-3632QM/corei7_sequential_256.txt" using ($1*1024):($2) title 'odczyt co 256B' with lines lw 2, \
"../corei7-3632QM/corei7_sequential_64.txt" using ($1*1024):($2) title 'odczyt co 64B' with lines lw 2, \
"../corei7-3632QM/corei7_sequential_8.txt" using ($1*1024):($2) title 'odczyt co 8B' with lines lw 2

