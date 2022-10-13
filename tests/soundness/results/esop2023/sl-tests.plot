set key autotitle columnhead
set xtics nomirror
set xrange [1:204]
set ytics nomirror
set border 3

# set term qt persist
set term pngcairo size 600, 400
set output "prover-tests.png"

# set title "Average-case Performance"

set xlabel "Prover Test Case"
set ylabel "Execution Time Overhead (%)"

# Spot: dasthtype 2
# SD: dashtype 5
# Rel: dashtype 6

plot 'prover-tests.dat' using 1 with lines dashtype 5 lc rgb "black", \
                     '' using 2 with lines dashtype 6 lc rgb "black", \
                     '' using 3 with lines dashtype 2 lc rgb "black"
