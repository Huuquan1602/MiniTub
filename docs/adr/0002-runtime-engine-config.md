# ADR-0002: Runtime EngineConfig instead of #ifdef variants

**Status:** accepted · **Date:** 2026-09-27

## Context
MiniTub exists to compare design trade-offs (replacer policy, index type, WAL
flush policy, ...). Compile-time switches would need one build per combination.

## Decision
All design choices are fields of `EngineConfig` (design.md section 6), loaded from
TOML. Parsing is strict: unknown keys, wrong types and bad enum values are errors,
so a typo never silently falls back to a default. Every benchmark result embeds
the full config.

## Consequences
- One release binary can sweep every configuration (`bench/sweep.py --configs ...`).
- Choosing a policy costs a virtual call or a branch at runtime. Hot paths
  (e.g. replacer calls) must be benchmarked to confirm the cost is negligible.
- `PAGE_SIZE` stays compile-time: it shapes on-disk layout and struct sizes.
