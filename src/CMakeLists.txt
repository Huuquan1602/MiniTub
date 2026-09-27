add_subdirectory(common)

find_package(Threads REQUIRED)

# Storage core (M1). One library: page guards and the buffer pool depend on each other.
add_library(minitub_storage STATIC
  storage/disk/disk_manager.cpp
  storage/disk/disk_scheduler.cpp
  storage/page/page_guard.cpp
  buffer/lru_k_replacer.cpp
  buffer/buffer_pool_manager.cpp)
minitub_target(minitub_storage)
target_link_libraries(minitub_storage PUBLIC minitub_common Threads::Threads)
