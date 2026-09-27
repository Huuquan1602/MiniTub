include(GoogleTest)

# minitub_add_test(<name> <source> [TIMEOUT <seconds>]): one GoogleTest binary, registered with CTest.
function(minitub_add_test name source)
  cmake_parse_arguments(ARG "" "TIMEOUT" "" ${ARGN})
  if(NOT ARG_TIMEOUT)
    set(ARG_TIMEOUT 60)
  endif()
  add_executable(${name} ${source})
  minitub_target(${name})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
  target_link_libraries(${name} PRIVATE minitub_common minitub_storage GTest::gtest_main)
  # Lets tests find repo files such as configs/default.toml.
  target_compile_definitions(${name} PRIVATE MINITUB_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
  # PRE_TEST: list tests when ctest runs, not at build time (sanitizer builds stay fast).
  # TIMEOUT: a buggy latch can deadlock; fail the test instead of hanging CI.
  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT ${ARG_TIMEOUT})
endfunction()

add_subdirectory(common)
add_subdirectory(storage)
add_subdirectory(buffer)
