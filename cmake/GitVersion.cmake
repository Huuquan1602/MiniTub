# Script mode (cmake -P), run on every build so results never carry a stale commit.
# Inputs: SRC (repo root), IN (template), OUT (generated header).
execute_process(COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${SRC} OUTPUT_VARIABLE GIT_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  set(GIT_COMMIT "unknown")
endif()

# Dirty = tracked files changed since the commit, so the hash alone does not describe the code.
execute_process(COMMAND git status --porcelain --untracked-files=no
  WORKING_DIRECTORY ${SRC} OUTPUT_VARIABLE git_changes ERROR_QUIET)
if(git_changes)
  set(GIT_DIRTY "true")
else()
  set(GIT_DIRTY "false")
endif()

# configure_file only touches OUT when the content changes, so nothing rebuilds needlessly.
configure_file(${IN} ${OUT} @ONLY)
