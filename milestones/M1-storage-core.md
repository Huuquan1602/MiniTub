# M1 — Storage core: disk manager, scheduler, LRU-K, buffer pool, page guards

**Status:** In progress · **Started:** 2026-09-27 · **Finished:** —
**Branch:** `m1` (CI is expected to be red until the tests pass)

## Goal
A thread-safe buffer pool over a page file, with counters for experiments E1/E2.
Interfaces and rules: [design.md section 7](../docs/design.md#7-storage-core-interfaces-m1).

## Definition of done
- [ ] disk_manager_test, disk_scheduler_test green
- [ ] lru_k_replacer_test green
- [ ] buffer_pool_manager_test, page_guard_test green
- [ ] buffer_pool_stress_test green under TSan
- [ ] debug/asan/tsan all green in CI, branch merged to main
- [ ] E1/E2 workload in `minitub_bench`

## What I did
- 2026-09-27: headers + stubs + failing tests (Claude); implementation: me

## Decisions (link ADRs)
-

## Problems & fixes
| Problem | Cause | Fix |
| --- | --- | --- |
|  |  |  |

## Verification
- Commit:
- CI run:
- Command to reproduce: `ctest --preset tsan -R "Disk|LRUK|BufferPool|PageGuard"`

## What I learned
-

## Open issues / carry over to M2
-
