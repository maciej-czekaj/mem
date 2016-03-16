
cmd="""
set style fill solid 1.00 border 0;
set style histogram errorbars gap 2 lw 1;
set style data histogram;
set grid ytics;
plot '$1' using 3:5:xtic(1) every 3 ti '0','' using 3:5:xtic(1) every 3::1 ti '16', '' using 3:5:xtic(1) every 3::2 ti '64';
pause -1
"""
echo "$cmd"
gnuplot -e "$cmd"
