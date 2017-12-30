#!/bin/bash
val=1; #procs
n=200000 # num elements 

#echo "n starts at " $n
#for i in $(seq 1 17);do # for each input size, run with changing processes
#val=1 # reset procs

for j in $(seq 1 8);do
echo $n
if [ $n -ge $val ]; then
    mpirun -np $val ./prog.out $n 5 
fi

    val=$(($val*2)) #vary procs
    n=$(($n*2)) # vary input size with procs
done
#echo ",,"
#n=$(($n*2)) # vary input size...
#done;
