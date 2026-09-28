# Roadmap

Scope of each milestone: [design.md section 4](design.md#4-milestones).
Each milestone ends with tests (debug/asan/tsan green) and a benchmark result
recorded in its `milestones/Mx-*.md` journal.

## M0: Project skeleton + benchmark harness
- [x] CMake presets: debug, asan, tsan, release; `-Wall -Wextra -Werror`
- [x] GoogleTest + Google Benchmark + toml++ + HdrHistogram + CLI11 via FetchContent
- [x] `src/common/`: config.h, macros.h, rid.h, exception.h, one test each
- [x] `EngineConfig` loaded from TOML, with tests for defaults, full load and invalid input
- [x] `minitub_bench` with a dummy workload, JSON report
- [x] `bench/sweep.py` + `bench/plot.py`
- [x] `.clang-format`, `.clang-tidy`, GitHub Actions (format + debug/asan/tsan)
- [x] README, ROADMAP

## M1: Storage core (disk + buffer pool)
- [ ] Headers, stubs and failing tests from design.md section 7
- [ ] Channel + DiskScheduler (per-page FIFO, clean shutdown)
- [ ] DiskManager: read/write/allocate 4 KiB pages, reopen, `storage.io_mode`
- [ ] LRUKReplacer
- [ ] BufferPoolManager: new/fetch/unpin/flush/delete, counters
- [ ] Page guards (basic/read/write)
- [ ] All M1 tests green on debug, asan, tsan (incl. 8-thread stress test)
- [ ] Benchmark workload for E1/E2

## M2: TableHeap
- [ ] Slotted page layout, tuple insert/delete/update, RID
- [ ] Table iterator
- [ ] Tests + insert/scan workload

## M3: B+ Tree index
- [ ] Insert, delete, point lookup, range scan (iterator)
- [ ] Latch crabbing for concurrent access
- [ ] Tests + lookup/insert workload

## M4: Extendible hash index
- [ ] Directory, bucket split/merge
- [ ] Tests + comparison against B+ tree (`index.default_type`)

## M5: Catalog + executors
- [ ] Catalog of tables, schemas, indexes
- [ ] Volcano executors: seq scan, index scan, insert, delete, update, filter, projection
- [ ] Nested-loop / hash join, aggregation, sort, limit

## M6: SQL front end
- [ ] Lexer + parser to AST
- [ ] Binder (names and types via catalog)
- [ ] Planner (AST to plan tree), CLI shell

## M7: Optimizer
- [ ] Rule-based rewrites (predicate pushdown, index selection, join order heuristics)

## M8: MVCC transactions
- [ ] Transaction manager, snapshot isolation (`transaction.isolation`)
- [ ] Undo logs, version chains, watermark GC
- [ ] Serializable validation

## M9: WAL + recovery
- [ ] Log manager, group commit (`wal.flush_policy`)
- [ ] Flush log before page (WAL rule)
- [ ] ARIES recovery: analysis, redo, undo
