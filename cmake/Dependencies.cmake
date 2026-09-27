include(FetchContent)

# Tarballs download faster than git clones. SYSTEM silences warnings in their headers.
FetchContent_Declare(googletest SYSTEM
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(tomlplusplus SYSTEM
  URL https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.tar.gz)
FetchContent_MakeAvailable(tomlplusplus)

# --- Benchmark-only dependencies ---
FetchContent_Declare(benchmark SYSTEM
  URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.tar.gz)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

# Latency histograms. Log file support is off, so zlib is not needed.
FetchContent_Declare(hdr_histogram SYSTEM
  URL https://github.com/HdrHistogram/HdrHistogram_c/archive/refs/tags/0.11.8.tar.gz)
set(HDR_LOG_REQUIRED DISABLED CACHE STRING "" FORCE)
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(HDR_HISTOGRAM_BUILD_SHARED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(cli11 SYSTEM
  URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.5.0.tar.gz)

FetchContent_MakeAvailable(benchmark hdr_histogram cli11)
