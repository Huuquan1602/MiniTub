include(GoogleTest)

# minitub_add_test(<name> <source>): one GoogleTest binary, registered with CTest.
function(minitub_add_test name source)
  add_executable(${name} ${source})
  minitub_target(${name})
  target_link_libraries(${name} PRIVATE minitub_common GTest::gtest_main)
  # Lets tests find repo files such as configs/default.toml.
  target_compile_definitions(${name} PRIVATE MINITUB_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
  # PRE_TEST: list tests when ctest runs, not at build time (sanitizer builds stay fast).
  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

add_subdirectory(common)
