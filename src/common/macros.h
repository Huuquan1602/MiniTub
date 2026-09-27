#pragma once

#include <cstdio>
#include <cstdlib>

namespace minitub::detail {

[[noreturn]] inline void AssertFail(const char *expr, const char *msg, const char *file, int line) {
  std::fprintf(stderr, "Assertion failed: %s (%s) at %s:%d\n", expr, msg, file, line);
  std::abort();
}

}  // namespace minitub::detail

// MT_ASSERT(cond, msg): abort with a message when cond is false.
// Like assert(), it is compiled out when NDEBUG is defined (release builds).
#ifdef NDEBUG
// sizeof keeps `cond` "used" (no unused-variable warnings) without evaluating it.
#define MT_ASSERT(cond, msg) static_cast<void>(sizeof(cond))
#else
#define MT_ASSERT(cond, msg)                                           \
  do {                                                                 \
    if (!(cond)) {                                                     \
      ::minitub::detail::AssertFail(#cond, (msg), __FILE__, __LINE__); \
    }                                                                  \
  } while (false)
#endif

// Put inside a class body to forbid copying.
#define DISALLOW_COPY(cname)     \
  cname(const cname &) = delete; \
  auto operator=(const cname &)->cname & = delete

// Put inside a class body to forbid copying and moving.
#define DISALLOW_COPY_AND_MOVE(cname) \
  DISALLOW_COPY(cname);               \
  cname(cname &&) = delete;           \
  auto operator=(cname &&)->cname & = delete
