set term png size 640,480; set output 'Rys7.png'; set logscale x 2; set format x "%.0b%B"; set logscale y 2; 
plot "../corei7-3632QM/corei7-mt_1" using ($1*1024):($2) title '1 wątek' with lines lw 2, \
"../corei7-3632QM/corei7-mt_2" using ($1*1024):($2) title '2 wątki' with lines lw 2, \
"../corei7-3632QM/corei7-mt_3" using ($1*1024):($2) title '3 wątki' with lines lw 2, \
"../corei7-3632QM/corei7-mt_5" using ($1*1024):($2) title '5 wątków' with lines lw 2

