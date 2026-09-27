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

## M1: DiskManager + DiskScheduler
- [ ] Read/write 4 KiB pages to `minitub.db`
- [ ] Disk scheduler: request queue + background worker thread
- [ ] `storage.io_mode` (buffered vs direct) honored
- [ ] Tests + sequential/random page I/O workload

## M2: Buffer pool
- [ ] Replacers: LRU, LRU-K, Clock (`buffer_pool.replacer`)
- [ ] BufferPoolManager: fetch/new/delete/flush, pin counts
- [ ] Read/write page guards (RAII)
- [ ] Tests + hit-rate / throughput workload per replacer

## M3: TableHeap
- [ ] Slotted page layout, tuple insert/delete/update, RID
- [ ] Table iterator
- [ ] Tests + insert/scan workload

## M4: B+ Tree index
- [ ] Insert, delete, point lookup, range scan (iterator)
- [ ] Latch crabbing for concurrent access
- [ ] Tests + lookup/insert workload

## M5: Extendible hash index
- [ ] Directory, bucket split/merge
- [ ] Tests + comparison against B+ tree (`index.default_type`)

## M6: Catalog + executors
- [ ] Catalog of tables, schemas, indexes
- [ ] Volcano executors: seq scan, index scan, insert, delete, update, filter, projection
- [ ] Nested-loop / hash join, aggregation, sort, limit

## M7: SQL front end
- [ ] Lexer + parser to AST
- [ ] Binder (names and types via catalog)
- [ ] Planner (AST to plan tree), CLI shell

## M8: Optimizer
- [ ] Rule-based rewrites (predicate pushdown, index selection, join order heuristics)

## M9: MVCC transactions
- [ ] Transaction manager, snapshot isolation (`transaction.isolation`)
- [ ] Undo logs, version chains, watermark GC
- [ ] Serializable validation

## M10: WAL + recovery
- [ ] Log manager, group commit (`wal.flush_policy`)
- [ ] Flush log before page (WAL rule)
- [ ] ARIES recovery: analysis, redo, undo
