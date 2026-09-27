# Regenerate git_version.h on every build (see cmake/GitVersion.cmake).
set(GIT_VERSION_H ${CMAKE_CURRENT_BINARY_DIR}/generated/git_version.h)
add_custom_target(minitub_git_version
  COMMAND ${CMAKE_COMMAND} -DSRC=${PROJECT_SOURCE_DIR} -DIN=${CMAKE_CURRENT_SOURCE_DIR}/git_version.h.in
          -DOUT=${GIT_VERSION_H} -P ${PROJECT_SOURCE_DIR}/cmake/GitVersion.cmake
  BYPRODUCTS ${GIT_VERSION_H})

add_executable(minitub_bench
  main.cpp
  histogram.cpp
  runner.cpp
  report.cpp
  system_info.cpp
  workload.cpp
  workloads/dummy_workload.cpp)
minitub_target(minitub_bench)
add_dependencies(minitub_bench minitub_git_version)
target_include_directories(minitub_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(minitub_bench PRIVATE minitub hdr_histogram_static CLI11::CLI11)
target_compile_definitions(minitub_bench PRIVATE
  MINITUB_BUILD_TYPE="${CMAKE_BUILD_TYPE}" MINITUB_SANITIZER_NAME="${MINITUB_SANITIZER}")

# Smoke test: runs the whole pipeline briefly with 2 threads (TSan checks the runner).
add_test(NAME bench_smoke
  COMMAND minitub_bench --workload=dummy --threads=2 --seconds=0.2
          --out=${CMAKE_CURRENT_BINARY_DIR}/bench_smoke.json)

# Google Benchmark micro-benchmarks for small, isolated pieces of code.
add_executable(minitub_micro micro/bm_dummy.cpp)
minitub_target(minitub_micro)
target_link_libraries(minitub_micro PRIVATE benchmark::benchmark_main)
