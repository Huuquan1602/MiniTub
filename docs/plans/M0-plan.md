# M0 Plan: Project skeleton + benchmark harness

Scope: build system, `src/common/`, `EngineConfig`, benchmark harness, CI,
docs. No database components (no disk manager, buffer pool, ...).
Design reference: [../design.md](../design.md).

## Machine setup

The project builds inside **WSL2 Ubuntu 24.04** on the Windows laptop
(same OS image as CI). The repo stays at `C:\Atticus\MiniTub` = `/mnt/c/Atticus/MiniTub`.

| Tool | Version | Source |
| --- | --- | --- |
| WSL2 Ubuntu | 24.04 | `wsl --install -d Ubuntu-24.04` |
| clang / clang-tidy | 18.1 | `apt install clang clang-tidy` |
| CMake | 3.28 | `apt install cmake` |
| Ninja | 1.11 | `apt install ninja-build` |
| Python | 3.12 | `apt install python3-venv` |
| matplotlib 3.11.2, clang-format 22.1.8 | pinned | `pip install -r bench/requirements.txt` in a venv |

TSan needs `sudo sysctl -w vm.mmap_rnd_bits=28` on this kernel (resets on reboot).

History: native Windows was tried first, but Visual Studio 18 was installed
without MSVC headers/libs or a Windows SDK, and TSan has no Windows support.
WSL fixed both.

## Decisions

- **Compiler: clang (18 locally and in CI).** The same `-Wall -Wextra -Werror`
  flags and sanitizers everywhere.
- **CMake >= 3.25** (satisfies ">= 3.20"): needed for `FetchContent_Declare(... SYSTEM)`,
  which stops our `-Werror` from breaking on third-party headers.
- **Warnings only on our targets** through an interface target (`minitub_warnings`),
  not global flags. **Sanitizer flags are global**, so dependencies are
  instrumented too (TSan needs that).
- **Flags parser:** CLI11. **JSON output:** toml++'s `json_formatter`, which
  avoids adding a separate JSON library.
- **Latency:** each thread records into its own HdrHistogram (ns), and the
  histograms are merged at the end. This avoids locking on the hot path.
- **Git hash:** regenerated at *build* time (only rewritten when it changes),
  so results never carry a stale commit. A `git_dirty` flag marks uncommitted code.
- **Sweep median:** `median_low` of the repeats, so the kept run is a real run
  and its latencies match its throughput.
- **clang-format pinned to 22.1.8** (pip) both locally and in CI, so the format
  check gives the same answer everywhere.
- **Bench smoke test** in ctest (2 threads, 0.2 s), so TSan also checks the runner.

## Dependencies (FetchContent, pinned)

GoogleTest v1.17.0 · Google Benchmark v1.9.4 · toml++ v3.4.0 ·
HdrHistogram_c 0.11.8 (log support off, no zlib) · CLI11 v2.5.0

## Steps (one commit each)

1. Build skeleton: `CMakeLists.txt`, `CMakePresets.json`, `cmake/*.cmake`, `.gitignore`
2. `src/common/` headers + one unit test per header
3. `EngineConfig` + TOML loader + tests + `configs/default.toml`
4. `bench/`: `minitub_bench`, dummy workload, JSON report, micro-bench,
   `sweep.py`, `plot.py`
5. `.clang-format`, `.clang-tidy`, `.github/workflows/ci.yml`
6. `README.md`, `docs/ROADMAP.md`, ADRs, tick `milestones/M0-setup.md`

## Definition of done

All verified locally in WSL on 2026-09-27 (27/27 tests per preset).


- [x] `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`
- [x] same for `asan`
- [x] same for `tsan` (with the `mmap_rnd_bits` fix)
- [x] `minitub_bench --workload=dummy --seconds=2 --out=r.json` gives valid JSON
- [x] `sweep.py` + `plot.py` produce a PNG from the dummy workload
- [x] `clang-format --dry-run` reports nothing
