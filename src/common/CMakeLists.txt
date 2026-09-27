add_library(minitub_common STATIC engine_config.cpp)
minitub_target(minitub_common)
target_include_directories(minitub_common PUBLIC ${PROJECT_SOURCE_DIR}/src)
# PUBLIC: engine_config_toml.h exposes toml::table to users.
target_link_libraries(minitub_common PUBLIC tomlplusplus::tomlplusplus)
