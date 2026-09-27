# ADR-0001: C++20, CMake presets, clang on Linux

**Status:** accepted · **Date:** 2026-09-27

## Context
MiniTub needs one-command builds for development, sanitizers and benchmarks,
on a Windows laptop and in CI.

## Decision
- C++20 (`std::latch`, `std::format`, defaulted `operator==`, concepts later).
- CMake >= 3.25 with `CMakePresets.json`: `debug`, `asan`, `tsan`, `release`.
  3.25 is needed for `FetchContent_Declare(... SYSTEM)`.
- clang + Ninja on Linux; on Windows the project builds inside WSL2 Ubuntu 24.04.
- Dependencies via FetchContent, pinned to release tarballs.

## Consequences
- ThreadSanitizer works locally (it has no Windows support) and matches CI (ubuntu-24.04, clang 18).
- Later disk code can use POSIX I/O (`pread`, `fsync`, `O_DIRECT`) directly.
- Building from `/mnt/c` in WSL is slower than a native Linux path; acceptable for now.
