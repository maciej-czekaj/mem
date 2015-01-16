set term png size 640,480; set output 'Rys2.png'; set logscale x 2; set format x "%.0b%B"; set logscale y 2; set xlabel "Rozmiar zbioru roboczego"; set ylabel "Czas dostępu (ns)"; \
 plot "../corei7-3632QM/corei7_random_128.txt" using ($1*1024):($2) title 'Corei-7 dostęp swobodny' with lines lw 2;
