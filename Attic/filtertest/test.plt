file = "test.log"

dB(x)=20*log10(x)

reset

set xlabel  "Frequency [Hz]"
set logscale x

set ylabel  "Gain [dB]"
set yrange [-120:20]            # 20 bit dynamic range
set ytics nomirror
# set logscale y

set y2label "Phase [deg]"
set y2range [-180:0]
set y2tics

set style data lines
set style line 1 linewidth 2
set style line 3 linewidth 2

set grid xtics mxtics
set grid ytics mytics

plot    file index 0    using 1:(dB($4))    title "Gain"  ls 1
replot  file index 0    using 1:($5)        title "Phase" ls 2 axes x1y2

replot  file index 1    using 1:(dB($4))    title "Gain"  ls 3
replot  file index 1    using 1:($5)        title "Phase" ls 4 axes x1y2

pause mouse close

