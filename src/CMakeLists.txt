# Same structure as BusTub: headers live in src/include/<module>/, sources in src/<module>/.
# Each module builds an OBJECT library; all of them are linked into one static library, `minitub`.

find_package(Threads REQUIRED)

# minitub_add_module(<name> <sources...>): one module's object library.
function(minitub_add_module name)
  add_library(${name} OBJECT ${ARGN})
  minitub_target(${name})
  target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/src/include)
  target_link_libraries(${name} PUBLIC tomlplusplus::tomlplusplus)
  # A global property collects the objects without PARENT_SCOPE chains through every directory.
  set_property(GLOBAL APPEND PROPERTY MINITUB_OBJECT_FILES $<TARGET_OBJECTS:${name}>)
endfunction()

add_subdirectory(buffer)
add_subdirectory(common)
add_subdirectory(storage)

get_property(MINITUB_OBJECT_FILES GLOBAL PROPERTY MINITUB_OBJECT_FILES)
add_library(minitub STATIC ${MINITUB_OBJECT_FILES})
target_include_directories(minitub PUBLIC ${PROJECT_SOURCE_DIR}/src/include)
# PUBLIC: engine_config_toml.h exposes toml::table to users.
target_link_libraries(minitub PUBLIC tomlplusplus::tomlplusplus Threads::Threads)
