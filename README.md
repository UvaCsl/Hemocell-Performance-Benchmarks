# Hemocell-Performance-Benchmarks
A repository to keep track of Hemocell performance benchmarks

For more information on Hemocell please go to [hemocell.eu](https://hemocell.eu/)

## How to use
1. Clone this repository in the hemocell root directory
2. Add the following lines to the bottom of the CMakelist.txt file. Change the folder name if nececary.
  ```
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Hemocell-Performance-Benchmarks")
          add_subdirectory(Hemocell-Performance-Benchmarks EXCLUDE_FROM_ALL)
  endif()
  ```
3. Benchmarks can now be compiled using the same procedure as normal cases.


## Setup a benchmark
Every benchmark lives in its own folder, and is setup the same as any other hemocell case.

Every benchmark folder must conatin a meta.yml file. This file contains information regarding the benchmark that will be stored for every experimint that is done with it.

The optional information is:
- name: the name of the benchmark
- version: the version of the benchmark

Any additional information is allowd, but keep in mind it will be copied for every experiment.


Additionaly a README.md should be added to every benchmark to provide more information.
