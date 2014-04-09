# Geo
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/burma14.tsp    burma14.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ulysses16.tsp  ulysses16.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ulysses22.tsp  ulysses22.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr96.tsp       gr96.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr137.tsp      gr137.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr202.tsp      gr202.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr229.tsp      gr229.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr431.tsp      gr431.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ali535.tsp     ali535.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr666.tsp      gr666.tour

# Att
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/att48.tsp      att48.tour		# short; correct SSE2, 10628
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/att532.tsp     att532.tour

# 2D
../vc11/SolveTSP/Release/SolveTSP.exe -amp-gpu TSP/eil51.tsp      eil51.tour		# char ; correct SSE2, 426, 89433 itr.; correct AMP-CM-Opt, 426, 79825 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/berlin52.tsp   berlin52.tour		# char ; correct SSE2, 7542, 109 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/st70.tsp       st70.tour		# char ; correct SSE2, 675, 255490 itr.; correct AMP-CM, 675, 197609 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/eil76.tsp      eil76.tour		# char ; correct SSE2, 538, 5948 itr.; correct AMP-CM, 538, 3356 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr76.tsp       pr76.tour		# short; correct SSE2, ?, ? itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rat99.tsp      rat99.tour		# short; correct SSE2, 1211, 10166 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rd100.tsp      rd100.tour		# float; correct SSE2, 7910, 61415 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroD100.tsp    kroD100.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroC100.tsp    kroC100.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroE100.tsp    kroE100.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroA100.tsp    kroA100.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroB100.tsp    kroB100.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/eil101.tsp     eil101.tour		# short; correct SSE2, 629, 101739 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/lin105.tsp     lin105.tour		# short; correct SSE2, 14379, 71120 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr107.tsp      pr107.tour		# short; correct SSE2, ?, ? itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr124.tsp      pr124.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/bier127.tsp    bier127.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ch130.tsp      ch130.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr136.tsp      pr136.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr144.tsp      pr144.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ch150.tsp      ch150.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroB150.tsp    kroB150.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroA150.tsp    kroA150.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr152.tsp      pr152.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u159.tsp       u159.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rat195.tsp     rat195.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d198.tsp       d198.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroA200.tsp    kroA200.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/kroB200.tsp    kroB200.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/ts225.tsp      ts225.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/tsp225.tsp     tsp225.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr226.tsp      pr226.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gil262.tsp     gil262.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr264.tsp      pr264.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/a280.tsp       a280.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr299.tsp      pr299.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/lin318.tsp     lin318.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/linhp318.tsp   linhp318.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rd400.tsp      rd400.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fl417.tsp      fl417.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr439.tsp      pr439.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pcb442.tsp     pcb442.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d493.tsp       d493.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u574.tsp       u574.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rat575.tsp     rat575.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/p654.tsp       p654.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d657.tsp       d657.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u724.tsp       u724.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rat783.tsp     rat783.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr1002.tsp     pr1002.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u1060.tsp      u1060.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/vm1084.tsp     vm1084.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pcb1173.tsp    pcb1173.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d1291.tsp      d1291.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl1304.tsp     rl1304.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl1323.tsp     rl1323.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/nrw1379.tsp    nrw1379.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fl1400.tsp     fl1400.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u1432.tsp      u1432.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fl1577.tsp     fl1577.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d1655.tsp      d1655.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/vm1748.tsp     vm1748.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u1817.tsp      u1817.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl1889.tsp     rl1889.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d2103.tsp      d2103.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u2152.tsp      u2152.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/u2319.tsp      u2319.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pr2392.tsp     pr2392.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pcb3038.tsp    pcb3038.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fl3795.tsp     fl3795.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fnl4461.tsp    fnl4461.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl5915.tsp     rl5915.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl5934.tsp     rl5934.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/rl11849.tsp    rl11849.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/usa13509.tsp   usa13509.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/brd14051.tsp   brd14051.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d15112.tsp     d15112.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/d18512.tsp     d18512.tour

# UPPER_ROW, FULL_MATRIX
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr17.tsp       gr17.tour		# short; correct SSE2, 2085, ? itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr21.tsp       gr21.tour		# short; correct SSE2, 2707, 73 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr24.tsp       gr24.tour		# short; correct SSE2, 1272, 715 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/fri26.tsp      fri26.tour		# short; correct SSE2, 937, 343 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/bayg29.tsp     bayg29.tour		# short; correct SSE2, 1610, 390 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/bays29.tsp     bays29.tour		# short; correct SSE2, 2020, 3427 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/dantzig42.tsp  dantzig42.tour	# short; correct SSE2, 699, 1572096 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/swiss42.tsp    swiss42.tour		# short; correct SSE2, 1273, 1156 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr48.tsp       gr48.tour		# short; correct SSE2, 5046, 2688909 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/hk48.tsp       hk48.tour		# short; correct SSE2, 11461, 1617 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/brazil58.tsp   brazil58.tour		# short; unknown
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/gr120.tsp      gr120.tour		# short; correct SSE2, 6942, 7664710 itr.
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/si175.tsp      si175.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/brg180.tsp     brg180.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/si535.tsp      si535.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pa561.tsp      pa561.tour
../vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/si1032.tsp     si1032.tour

# Ceil 2D
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/dsj1000.tsp    dsj1000.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pla7397.tsp    pla7397.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pla33810.tsp   pla33810.tour
#./vc11/SolveTSP/Release/SolveTSP.exe -cpu-vpu TSP/pla85900.tsp   pla85900.tour
