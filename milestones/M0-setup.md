# M0 — Project skeleton + benchmark harness

**Status:** In progress · **Started:** 2026-09-28 · **Finished:** —

## Goal
Every later milestone can be built, tested, sanitized and benchmarked
with one command, without revisiting the setup.

## Definition of done
- [ ] `cmake --preset debug && cmake --build --preset debug && ctest` green locally
- [ ] Presets: debug, asan, tsan, release
- [ ] CI (GitHub Actions) runs build + tests on every push, asan + tsan jobs included
- [ ] clang-format + clang-tidy config, CI fails on format errors
- [ ] `src/common/`: config.h, macros.h, rid.h, exception.h
- [ ] `EngineConfig` struct + load from TOML (all fields from design doc §16)
- [ ] `bench/`: `minitub_bench` binary accepts --workload/--config/--threads/--out,
      writes result JSON (even with a dummy workload)
- [ ] Google Benchmark + HdrHistogram wired in
- [ ] `bench/sweep.py` + `bench/plot.py` run end-to-end on the dummy workload

## What I did
- 2026-09-28: ...

## Decisions (link ADRs)
- ADR-0001: C++20 + CMake presets
- ADR-0002: runtime `EngineConfig` instead of `#ifdef` variants

## Problems & fixes
| Problem | Cause | Fix |
| --- | --- | --- |
|  |  |  |

## Verification
- Commit: `abc1234`
- CI run: <link>
- Command to reproduce: `...`

## What I learned
-

## Open issues / carry over to M1
-