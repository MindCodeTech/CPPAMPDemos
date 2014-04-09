#This awk script merges the results.csv file with a results history csv file
#for easy importation into spreadsheet programs and analysis. Run in the same
#folder as the results.csv file and give awk the history file, and save the
#output as the new history file. A results.csv file works as a initial history
#file.
#
#Example usage:
#    awk -f merge_results.csv results-history.csv > new-history.csv

BEGIN {
  while (err=getline < "results.csv") {
    if (err == -1) {
      print "ERROR"
      exit -1
    }
    split($0,a,",")
    str=$0
    sub(a[1]",","",str)
    values[a[1]]=str
  }
}

{
  len = split($0,a,",")
  if (values[a[1]]) {
    print $0 "," values[a[1]]
  } else {
    print $0 ",FAILED"
  }
  delete values[a[1]]
}

END {
  for (v in values) {
    printf v
    for (i=0; i<len; i++) {
      printf ","
    }
    print values[v]
  }
}
