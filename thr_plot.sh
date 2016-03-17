
cmd="""
set term png size 1280,768;
set output '$1.png';
set style fill solid 1.00 border -1;
set style histogram errorbars gap 1 lw 2;
set style data histogram;
set grid ytics;
plot '$1' using 3:5:xtic(1) every 3 ti '0','' using 3:5:xtic(1) every 3::1 ti '16', '' using 3:5:xtic(1) every 3::2 ti '64';
"""
echo "$cmd" | gnuplot -
