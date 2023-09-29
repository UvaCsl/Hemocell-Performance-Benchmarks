# cube imbalance domain decomposition

Benchmark where incorrect domain decomposition causes load imbalance.

##
In the current version of the benchmark, the number of atomic blocks generated is three times the number of processes.
The first two-thirds of blocks are given to the first half of the available processes. 
![ ](Cube-example.png  "Imbalance domain visual representation")
In this examples the red subdomain is assigned to half the available processes, and the blue part is assigned to the other half.