# M0 — Project skeleton + benchmark harness

**Status:** Done (CI not run yet: no GitHub remote) · **Started:** 2026-09-27 · **Finished:** 2026-09-27

## Goal
Every later milestone can be built, tested, sanitized and benchmarked
with one command, without revisiting the setup.

## Definition of done
- [x] `cmake --preset debug && cmake --build --preset debug && ctest` green locally
- [x] Presets: debug, asan, tsan, release
- [x] CI (GitHub Actions) runs build + tests on every push, asan + tsan jobs included
      (workflow written; first run happens on the first push)
- [x] clang-format + clang-tidy config, CI fails on format errors
- [x] `src/common/`: config.h, macros.h, rid.h, exception.h
- [x] `EngineConfig` struct + load from TOML (all fields from design doc §6)
- [x] `bench/`: `minitub_bench` binary accepts --workload/--config/--threads/--out,
      writes result JSON (even with a dummy workload)
- [x] Google Benchmark + HdrHistogram wired in
- [x] `bench/sweep.py` + `bench/plot.py` run end-to-end on the dummy workload

## What I did
- 2026-09-27: Wrote docs/design.md (layout, milestones, EngineConfig) and
  docs/plans/M0-plan.md. Moved the build to WSL2 Ubuntu 24.04. Built the CMake
  skeleton, common headers, EngineConfig, bench harness, CI, README, ROADMAP.

## Decisions (link ADRs)
- [ADR-0001](../docs/adr/0001-cpp20-cmake-presets.md): C++20 + CMake presets, clang on Linux/WSL
- [ADR-0002](../docs/adr/0002-runtime-engine-config.md): runtime `EngineConfig` instead of `#ifdef` variants

## Problems & fixes
| Problem | Cause | Fix |
| --- | --- | --- |
| clang on Windows: `cannot open msvcrtd.lib` | VS 18 installed without MSVC libs/headers or Windows SDK | Build in WSL2 Ubuntu instead |
| TSan: "unexpected memory mapping" | kernel `vm.mmap_rnd_bits=32`, clang 18 TSan expects <= 28 | `sysctl -w vm.mmap_rnd_bits=28` (local + CI step) |
| CMake: `INSTALL(EXPORT) given unknown export "hdr_histogram-targets"` | HdrHistogram exports nothing when static install is off | Keep `HDR_HISTOGRAM_INSTALL_STATIC` at its default |
| HdrHistogram pulled in zlib | `HDR_LOG_REQUIRED` defaults to ON | Set it to `DISABLED` (we never write HdrHistogram log files) |
| clang-tidy flagged toml++ headers | `(src\|test\|bench)/` also matches `tomlplusplus-src/` | Filter `'/(src\|test\|bench)/'` |

## Verification
- Commit: `325d100` (code), followed by this docs commit
- CI run: not yet (repository has no remote)
- Command to reproduce (in WSL, repo root):
  `cmake --preset asan && cmake --build --preset asan && ctest --preset asan` (same for debug, tsan)
- Results: 27/27 tests pass on debug, asan and tsan; 0 compiler warnings.
- Bench (release, dummy workload, 1 thread, 2 s): ~12.0 M ops/s, p99 66 ns.
  Measured on Intel Core Ultra 7 255H, 16 logical cores, 15 GiB, WSL2 kernel 6.6.
  This number only checks the harness; it says nothing about the database.

## What I learned
- The sanitizer flags must reach dependencies too (TSan), while `-Werror` must not.
- A git hash captured at configure time goes stale; regenerate it each build.
- Median of an odd number of repeats keeps a real run, so latency and throughput match.

## Open issues / carry over to M1
- Push to GitHub and confirm the CI matrix is green.
- clang-tidy is advisory; consider making it a CI gate once the code base grows.
- Timing every Op() with `steady_clock` costs roughly 30 ns (estimate: the dummy op's p50 is 36 ns); fine for page-level work,
  but too coarse for nanosecond-scale ops (use Google Benchmark for those).
