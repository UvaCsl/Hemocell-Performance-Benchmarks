# Hemocell-Performance-Benchmarks
A repository to keep track of Hemocell performance benchmarks

For more information on Hemocell please go to [hemocell.eu](https://hemocell.eu/)

Mandatory extra files required for running hemocell can be found in `/misc`.

## Benchmarks
| Name                         	| Description                                                                                                                                                                       	| Status             	|
|------------------------------	|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	|--------------------	|
| Cube-Benchmark               	| The default cube example, a cubic volume of blood with a shear force applied to the top and bottom. This benchmark is used as basis for the other cube benchmarks.                	| :white_check_mark: 	|
| cube-imbalance-domain-decomp 	| A benchmark where workload imbalance is introduced in the cube case. Through domain decomposition, half of the available process are given 2 times more work than the other half. 	| :white_check_mark: 	|
| cube-imbalance-hemo          	| A benchmark where workload imbalance is introduced by having an imbalanced RBC distribution. Imbalanced distribution is created through the RBC.pos files.                        	| :white_check_mark: 	|
| rebalancing-cost             	| A benchmark for measuring the cost of rebalancing the workload, based on the cube-benchmark.                                                                                       	| :white_check_mark: 	|
| flow-part-filled             	| A benchmark with a flow direction. A cluster of RBCs moves through the domain, which creates dynamically changing workload distribution.                                          	| :red_circle:       	|

## Instructions

### How to use
1. Clone this repository in the hemocell root directory
2. Add the following lines to the bottom of the CMakelist.txt file. Change the folder name if necessary.
  ```
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Hemocell-Performance-Benchmarks")
          add_subdirectory(Hemocell-Performance-Benchmarks EXCLUDE_FROM_ALL)
  endif()
  ```
3. Benchmarks can now be compiled using the same procedure as normal cases.
    - Rerun CMAKE in `hemocell/build`
  
### Config.xml options for benchmarks
Hemocell requires a config file that is used to setup the simulation. Some benchmark specific options can be set using this config file. 
Here we will describe the options used for most/all benchmarks. Some special options might be availible to specific benchmarks, if this is the case they will be described in the benchmark README.

```
<benchmark>
    <binSize> 200 </binSize> <!----Set the bin size for binning iterarations over time using SCOREP. A higher value will provide less detail, but also less overhead. Default: tmax + 1 --->
    <writeOutput> 1 </writeOutput <!---Set to 0 if you don't want to write the ouptut of the simulation to hdf5 files, this reduces the diskspace required per experiment. Default: 1.--->
</benchmark>
```


### Setup a benchmark
Every benchmark lives in its own folder, and is set up the same as any other hemocell case.

Every benchmark folder must contain a meta.yml file. This file contains information regarding the benchmark that will be stored for every experiment that is done with it.

The optional information is:
- name: the name of the benchmark
- version: the version of the benchmark

Any additional information is allowed, but keep in mind it will be copied for every experiment.

Additionally, a README.md should be added to every benchmark to provide more information.

## Performance Monitoring
- Profiler
- ear

### ScoreP
ScoreP is an instrumentation tool that is part of the Scalasca tool chain, [https://www.vi-hps.org/projects/score-p/](https://www.vi-hps.org/projects/score-p/) [https://www.scalasca.org/](https://www.scalasca.org/).

If ScoreP is installed on your platform, it can be used to measure the performance of hemocell
To use it you must use the following CMAKE command when creating the build system for Hemocell.
```
SCOREP_WRAPPER=off SCOREP_WRAPPER_INSTRUMENTER_FLAGS=--user cmake ../ -DCMAKE_C_COMPILER=scorep-gcc -DCMAKE_CXX_COMPILER=scorep-g++ -DMPI_CXX_COMPILER=scorep-mpicxx
```

Use the following command to build the cube-benchmark case (while still in hemocell/build).
```
SCOREP_WRAPPER_INSTRUMENTER_FLAGS=--user make cube-benchmark
```

Set the following environment variables.
- The filter included in this repository will only measure a select few functions. If no filter is included, execution will become very slow.
```
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_FILTERING_FILE=filter.filter
export SCOREP_EXPERIMENT_DIRECTORY=SCOREP-jobname
export SCOREP_VERBOSE=false
```

For more information please look at the SCOREP [documentation](https://www.vi-hps.org/projects/score-p/).
