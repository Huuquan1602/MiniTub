# Link this into our own targets only; third-party code keeps its own flags.
add_library(minitub_warnings INTERFACE)
target_compile_options(minitub_warnings INTERFACE -Wall -Wextra -Werror)

if(MINITUB_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

# Apply the per-target settings every MiniTub target shares.
function(minitub_target target)
  target_link_libraries(${target} PRIVATE minitub_warnings)
  if(MINITUB_CLANG_TIDY)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
  endif()
endfunction()
