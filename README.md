# Hemocell-Performance-Benchmarks
A repository to keep track of Hemocell performance benchmarks

For more information on Hemocell please go to [hemocell.eu](https://hemocell.eu/)

Mandatory extra files required for running hemocell can be found in `/misc`.

## How to use
1. Clone this repository in the hemocell root directory
2. Add the following lines to the bottom of the CMakelist.txt file. Change the folder name if necessary.
  ```
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Hemocell-Performance-Benchmarks")
          add_subdirectory(Hemocell-Performance-Benchmarks EXCLUDE_FROM_ALL)
  endif()
  ```
3. Benchmarks can now be compiled using the same procedure as normal cases.
    - Rerun CMAKE in `hemocell/build`


## Setup a benchmark
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
