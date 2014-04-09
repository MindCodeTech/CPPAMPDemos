working_benchmarks='
BinomialOptions_256.cpp
FloydWarshall.cpp
CopyTest.cpp
CreateDestroy_AcceleratorView.cpp
CreateDestroy_Container.cpp
InTileScan.cpp'

rm Benchmarks/*.out
if [ -f report.txt ]; then
  rm report.txt
fi
if [ -f results.csv ]; then
  rm results.csv
fi

for a in $working_benchmarks; do
  if  ./buildme Benchmarks/$a; then
    echo "Compiled $a" >> report.txt
  else
    echo  "$a did not compile." >> report.txt
  fi
done
echo "commit,$(git rev-parse --short HEAD)" >> results.csv
echo "date,$(date "+%D %T")" >> results.csv

for a in $(ls Benchmarks/*.out) ; do
  name=$(basename $a)
  echo "running $name..."
  echo "Running $name:" >> report.txt
  if $a > tmp_output ; then
    cat tmp_output >> report.txt
    cat tmp_output | \
      awk '
        /^time/ {results[$2] = $3}
        /^fail/ {results[$2] = FAILED}
        END {
          for (n in results) {
            print n "," results[n]
          }
        }
      ' >> results.csv
  else
    echo "$name failed" >> report.txt
  fi
done
rm tmp_output
