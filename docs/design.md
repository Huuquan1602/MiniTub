# MiniTub Design

MiniTub is a disk-oriented relational DBMS written from scratch in C++20.
BusTub (CMU 15-445) is the behavioral reference; no BusTub source is copied.
The component diagram lives in [architectures.md](architectures.md).

## 1. Goals

- Learn how a DBMS works by building each layer bottom-up.
- Correctness first: every module ships with unit tests and runs clean
  under ASan and TSan.
- Measure trade-offs: every module gets a benchmark workload, and design
  choices are runtime options (`EngineConfig`), not `#ifdef` variants,
  so they can be compared with one binary.

## 2. Directory layout

```
cmake/          build helpers (warnings, sanitizers, dependencies, git version)
configs/        TOML engine configs (default.toml)
src/common/     shared types: config.h, macros.h, rid.h, exception.h, engine_config.*
src/storage/    disk/ (M1), table/ (M3), index/ (M4, M5)
src/buffer/     buffer pool + replacers (M2)
src/catalog/    catalog (M6)
src/execution/  Volcano executors (M6)
src/sql/        parser, binder, planner, optimizer (M7, M8)
src/concurrency/ MVCC transactions (M9)
src/recovery/   WAL + ARIES (M10)
test/           mirrors src/ (test/common/..., test/buffer/...)
bench/          minitub_bench harness, workloads/, micro/, sweep.py, plot.py
docs/           design.md, ROADMAP.md, plans/, adr/
milestones/     per-milestone journals (M0-setup.md, ...)
```

A directory is created by the milestone that first needs it.

## 3. Build and tooling

- C++20, CMake >= 3.25, Ninja, clang. Presets: `debug`, `asan`, `tsan`, `release`.
- Our targets compile with `-Wall -Wextra -Werror`; third-party code does not.
- Dependencies come from FetchContent, pinned to tags: GoogleTest, Google
  Benchmark, toml++, HdrHistogram_c, CLI11.
- Formatting: clang-format 22 (`.clang-format`). Static analysis: `.clang-tidy`.
- CI: GitHub Actions on Linux; format check + debug/asan/tsan build and test.

## 4. Milestones

| ID  | Scope |
| --- | --- |
| M0  | Project skeleton, `EngineConfig`, benchmark harness, CI |
| M1  | DiskManager + DiskScheduler (I/O queue, worker thread) |
| M2  | BufferPool: LRU / LRU-K / Clock replacers, page guards |
| M3  | TableHeap: slotted pages, tuples, RID |
| M4  | B+ Tree index |
| M5  | Extendible hash index |
| M6  | Catalog + Volcano executors (scan, insert, delete, filter, join, agg) |
| M7  | SQL front end: lexer/parser, binder, planner |
| M8  | Rule-based optimizer |
| M9  | MVCC transactions (undo logs, watermark GC) |
| M10 | WAL (group commit) + ARIES recovery |

Checklists live in [ROADMAP.md](ROADMAP.md); journals in `milestones/`.

## 5. Testing and benchmarking

- Unit tests: GoogleTest, one test file per header/module, run via `ctest`.
- Micro-benchmarks: Google Benchmark (`bench/micro/`).
- Workload benchmarks: `minitub_bench --workload=<name> --config=<toml>
  --threads=N --seconds=S --seed=X --out=result.json`.
  Output JSON holds throughput, p50/p95/p99 latency (HdrHistogram, ns),
  the full `EngineConfig`, git commit, and CPU/RAM info.
- `bench/sweep.py` runs a config matrix, 5 repeats per point, keeps the median.
  `bench/plot.py` turns sweep results into PNG charts.
- Every reported result records workload, machine, runs, and baseline.

## 6. EngineConfig

Runtime options, loaded from TOML. Unknown sections/keys, wrong value types,
and unknown enum values are errors. `PAGE_SIZE` is a compile-time constant
(`config.h`), not a config key.

| Key | Type | Default | Values |
| --- | --- | --- | --- |
| `storage.db_file` | string | `"minitub.db"` | |
| `storage.log_file` | string | `"minitub.log"` | |
| `storage.io_mode` | enum | `buffered` | `buffered`, `direct` |
| `storage.disk_scheduler_workers` | int | `1` | >= 1 |
| `buffer_pool.pool_size` | int | `1024` | frames, >= 1 |
| `buffer_pool.replacer` | enum | `lru_k` | `lru`, `lru_k`, `clock` |
| `buffer_pool.lru_k` | int | `2` | >= 1 |
| `index.default_type` | enum | `bplus_tree` | `bplus_tree`, `extendible_hash` |
| `transaction.isolation` | enum | `snapshot` | `snapshot`, `serializable` |
| `wal.enabled` | bool | `false` | |
| `wal.flush_policy` | enum | `group_commit` | `every_commit`, `group_commit` |
| `wal.group_commit_interval_us` | int | `1000` | >= 1 |

Example: [configs/default.toml](../configs/default.toml). Every key is
optional; missing keys keep their defaults.
