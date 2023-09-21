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
