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
src/storage/
  disk/         disk_manager, channel, disk_scheduler (M1)
  page/         page, page_guard (M1), table_page (M2)
  table/        table_heap (M2)
  index/        B+ tree (M3), extendible hash (M4)
src/buffer/     replacer, lru_k_replacer, buffer_pool_manager (M1)
src/catalog/    catalog (M5)
src/execution/  Volcano executors (M5)
src/sql/        parser, binder, planner, optimizer (M6, M7)
src/concurrency/ MVCC transactions (M8)
src/recovery/   WAL + ARIES (M9)
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
| M1  | Storage core: DiskManager, DiskScheduler, LRU-K replacer, BufferPoolManager, page guards |
| M2  | TableHeap: slotted pages, tuples, RID |
| M3  | B+ Tree index |
| M4  | Extendible hash index |
| M5  | Catalog + Volcano executors (scan, insert, delete, filter, join, agg) |
| M6  | SQL front end: lexer/parser, binder, planner |
| M7  | Rule-based optimizer |
| M8  | MVCC transactions (undo logs, watermark GC) |
| M9  | WAL (group commit) + ARIES recovery |

Checklists live in [ROADMAP.md](ROADMAP.md); journals in `milestones/`.
(Renumbered at the start of M1: disk and buffer pool merged into one milestone.)

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

## 7. Storage core interfaces (M1)

Headers are the contract; the tests in `test/storage/` and `test/buffer/` check
exactly the rules below. All I/O errors throw `Exception(ExceptionType::Io)`.

### 7.1 DiskManager (`src/storage/disk/disk_manager.h`)

```cpp
DiskManager(const std::filesystem::path &file, IoMode mode = IoMode::Buffered);
void ReadPage(page_id_t id, std::span<std::byte, PAGE_SIZE> out);
void WritePage(page_id_t id, std::span<const std::byte, PAGE_SIZE> data);
auto AllocatePage() -> page_id_t;
void DeallocatePage(page_id_t id);
auto NumPages() const -> page_id_t;
auto GetStats() const -> DiskStats;   // {reads, writes}
```

- Opens the file, creating it if missing. An existing file keeps its pages:
  `NumPages() == file_size / PAGE_SIZE`.
- `AllocatePage()` returns `NumPages()` and grows the file by one zeroed page,
  so ids are 0, 1, 2, ... and survive a reopen.
- Reading an allocated page that was never written returns zeros.
- `ReadPage`/`WritePage` with `id < 0` or `id >= NumPages()` throw `OutOfRange`.
- `DeallocatePage` only records the id in M1; ids are never reused.
- `IoMode::Direct` opens with `O_DIRECT`; callers must pass `PAGE_SIZE`-aligned buffers.
- Thread-safe: several scheduler workers may call it at once.
- Counters: `reads`/`writes` = number of `ReadPage`/`WritePage` calls.

### 7.2 Channel and DiskScheduler (`src/storage/disk/channel.h`, `disk_scheduler.h`)

```cpp
template <typename T> class Channel { void Put(T v); auto Get() -> T; };
struct DiskRequest { bool is_write; std::byte *data; page_id_t page_id; std::promise<bool> callback; };
DiskScheduler(DiskManager *dm, int num_workers = 1);
~DiskScheduler();
void Schedule(DiskRequest r);
```

- `Channel`: unbounded FIFO. `Put` never blocks; `Get` blocks until an item exists.
  Safe for many producers and consumers; every item is delivered exactly once.
- `Schedule` never blocks on I/O. A worker performs the request, then sets
  `callback` to `true` (success) or `false` (the DiskManager threw).
- `data` must stay valid until the callback is set.
- Requests for the same page complete in the order they were scheduled, for any
  `num_workers` (route by `page_id % num_workers` to one channel per worker).
- The destructor finishes every request already scheduled, then stops and joins
  the workers (one `std::nullopt` per worker channel as the stop signal).

### 7.3 Page (`src/storage/page/page.h`)

A frame of the buffer pool: `alignas(PAGE_SIZE)` data, `page_id`, `pin_count`,
`is_dirty`, `page_lsn` (unused until M9), and a reader/writer latch
(`RLatch/RUnlatch/WLatch/WUnlatch`). Accessors are trivial; only
`BufferPoolManager` (a friend) changes the metadata, under its own latch.

### 7.4 Replacer and LRUKReplacer (`src/buffer/replacer.h`, `lru_k_replacer.h`)

```cpp
LRUKReplacer(size_t num_frames, size_t k);
auto Evict() -> std::optional<frame_id_t>;
void RecordAccess(frame_id_t f);
void SetEvictable(frame_id_t f, bool evictable);
void Remove(frame_id_t f);
auto Size() const -> size_t;
```

- Every `RecordAccess` gets the next value of a logical clock (0, 1, 2, ...).
  Only the last `k` timestamps of a frame are kept.
- A frame seen for the first time is tracked and **not evictable**.
- Backward k-distance = now − (k-th most recent timestamp). Frames with fewer
  than `k` accesses have distance +inf.
- `Evict` picks, among evictable frames, the largest distance. Ties at +inf go to
  the frame whose **oldest recorded access** is earliest. The evicted frame's
  history is dropped (a later access starts fresh). Returns `nullopt` if nothing
  is evictable.
- `SetEvictable` on an untracked frame does nothing; repeating the same value
  does not change `Size()`.
- `Remove` drops an evictable frame's history; on an untracked frame it does
  nothing; on a non-evictable frame it throws `Invalid`.
- `RecordAccess`/`SetEvictable`/`Remove` with `f < 0` or `f >= num_frames` throw `OutOfRange`.
- `Size()` = number of evictable frames. All methods are thread-safe.

### 7.5 BufferPoolManager (`src/buffer/buffer_pool_manager.h`)

```cpp
BufferPoolManager(size_t pool_size, DiskManager *dm, size_t replacer_k = 2, int disk_workers = 1);
auto NewPage() -> Page *;                        // nullptr: no free or evictable frame
auto FetchPage(page_id_t id) -> Page *;          // nullptr: no free or evictable frame
auto UnpinPage(page_id_t id, bool is_dirty) -> bool;
auto FlushPage(page_id_t id) -> bool;
void FlushAllPages();
auto DeletePage(page_id_t id) -> bool;
auto NewPageGuarded() -> BasicPageGuard;
auto FetchPageBasic(page_id_t id) -> BasicPageGuard;
auto FetchPageRead(page_id_t id) -> ReadPageGuard;
auto FetchPageWrite(page_id_t id) -> WritePageGuard;
auto FreeFrameCount() const -> size_t;
auto GetStats() const -> BufferPoolStats;        // {hits, misses, evictions}
```

- All I/O goes through an owned `DiskScheduler`.
- Frames come from the free list first, then from `Replacer::Evict()`.
  An evicted dirty page is written back first; a clean one is not written.
  `// M9: flush log up to page_lsn` marks where WAL hooks into eviction.
- `NewPage`: allocates a page id only after a frame is secured; the page is zeroed,
  pinned once, not dirty.
- `FetchPage`: resident → pin +1 (a **hit**); otherwise read from disk into a frame,
  pin = 1 (a **miss**). Every successful fetch/new records an access and makes the
  frame non-evictable.
- `UnpinPage`: `false` if the page is not resident or its pin count is 0. The dirty
  flag is sticky (`is_dirty=false` never clears it). At pin 0 the frame becomes evictable.
- `FlushPage`: `false` if not resident; otherwise writes the page (dirty or not)
  and clears the dirty flag.
- `DeletePage`: `false` if pinned; `true` if not resident; otherwise frees the frame
  (back to the free list) without writing it, and deallocates the page id.
- Counters: `hits`/`misses` per `FetchPage`; `evictions` per frame taken from the replacer.
- Thread-safe.

### 7.6 Page guards (`src/storage/page/page_guard.h`)

RAII holders of a pinned page. Move-only; a moved-from guard is empty and does
nothing. `Drop()` releases early and is idempotent; the destructor calls it.

- `BasicPageGuard`: pin only. `GetDataMut()` marks the page dirty.
- `ReadPageGuard`: pin + shared latch. `Drop()` releases the **latch first, then unpins**.
- `WritePageGuard`: pin + exclusive latch. `GetDataMut()` marks dirty. Same drop order.
- A guard from a failed fetch (no frame) is empty: `IsValid() == false`.
- Move-assigning into a guard first drops what it held.

## 8. Experiments

| ID | Question | Knobs | Metrics |
| --- | --- | --- | --- |
| E1 | How do replacers compare as the pool shrinks? | `buffer_pool.replacer`, `pool_size`, access skew | hit rate, throughput, evictions |
| E2 | What do more I/O workers and direct I/O buy? | `storage.disk_scheduler_workers`, `storage.io_mode` | throughput, p99, disk reads/writes |

The counters in `DiskStats` and `BufferPoolStats` exist for these experiments.
