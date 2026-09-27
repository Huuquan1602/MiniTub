# M0 Plan: Project skeleton + benchmark harness

Scope: build system, `src/common/`, `EngineConfig`, benchmark harness, CI,
docs. No database components (no disk manager, buffer pool, ...).
Design reference: [../design.md](../design.md).

## Machine setup (Windows 11)

| Tool | Version | Source |
| --- | --- | --- |
| CMake | 4.3 | already installed |
| clang / clang-tidy | 22.1 | already installed (`C:\Program Files\LLVM\bin`) |
| MSVC headers/libs | 14.51 | Visual Studio 18 Community (clang uses them) |
| Ninja | latest | `winget install Ninja-build.Ninja` |
| Python | 3.12 | already installed |
| matplotlib, clang-format 22.1.8 | pinned | `.venv` via `pip install -r bench/requirements.txt` |

Both `C:\Program Files\LLVM\bin` and Ninja must be on `PATH`.

Limitation: ThreadSanitizer does not support Windows, so the `tsan` preset only
runs on Linux (it is hidden on Windows). CI (ubuntu-24.04) covers it. To run it
locally, use WSL: `wsl --install -d Ubuntu-24.04`.

## Decisions

- **Compiler: clang everywhere.** One compiler across Windows, Linux and CI
  means the same `-Wall -Wextra -Werror` flags and the same sanitizers.
- **CMake >= 3.25** (satisfies ">= 3.20"): needed for `FetchContent_Declare(... SYSTEM)`,
  which stops our `-Werror` from breaking on third-party headers.
- **Warnings only on our targets** through an interface target (`minitub_warnings`),
  not global flags.
- **Windows CRT:** everything is built against the release DLL CRT (`/MD`),
  because ASan on Windows does not support the debug CRT.
- **Flags parser:** CLI11. **JSON output:** toml++'s `json_formatter`, which
  avoids adding a separate JSON library.
- **Latency:** each thread records into its own HdrHistogram (ns), and the
  histograms are merged at the end. This avoids locking on the hot path.
- **Git hash:** regenerated at *build* time (only rewritten when it changes),
  so results never carry a stale commit.
- **clang-format pinned to 22.1.8** (pip) both locally and in CI, so the format
  check gives the same answer everywhere.

## Dependencies (FetchContent, pinned)

GoogleTest v1.17.0 · Google Benchmark v1.9.4 · toml++ v3.4.0 ·
HdrHistogram_c 0.11.8 (zlib/log support off) · CLI11 v2.5.0

## Steps (one commit each)

1. Build skeleton: `CMakeLists.txt`, `CMakePresets.json`, `cmake/*.cmake`, `.gitignore`
2. `src/common/` headers + one unit test per header
3. `EngineConfig` + TOML loader + tests + `configs/default.toml`
4. `bench/`: `minitub_bench`, dummy workload, JSON report, micro-bench,
   `sweep.py`, `plot.py`
5. `.clang-format`, `.clang-tidy`, `.github/workflows/ci.yml`
6. `README.md`, `docs/ROADMAP.md`, ADRs, tick `milestones/M0-setup.md`

## Definition of done

- [ ] `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`
- [ ] same for `asan`
- [ ] same for `tsan` (CI only, see limitation above)
- [ ] `minitub_bench --workload=dummy --seconds=2 --out=r.json` gives valid JSON
- [ ] `sweep.py` + `plot.py` produce a PNG from the dummy workload
- [ ] `clang-format --dry-run` reports nothing
