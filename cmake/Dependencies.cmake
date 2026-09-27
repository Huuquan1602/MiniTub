include(FetchContent)

# Tarballs download faster than git clones. SYSTEM silences warnings in their headers.
FetchContent_Declare(googletest SYSTEM
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
