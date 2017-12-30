#!/bin/bash
val=1; #procs
n=8 #dimensions
for i in $(seq 1 9);do # for each input size, run with changing processes
val=1 # reset procs
echo "*********************************"
for j in $(seq 1 7);do
if [ $n -ge $val ]; then
mpirun -np $val ./gol.out $n $n 400
fi
val=$(($val*2))
done
n=$(($n*2)) # vary input size...
echo "*******************************"
#rm file*
done;
