# MiniTub

A disk-oriented relational database written from scratch in C++20, modeled on
CMU 15-445's [BusTub](https://github.com/cmu-db/bustub) (behavior reference only, no copied code).

- Design: [docs/design.md](docs/design.md) · Architecture: [docs/architectures.md](docs/architectures.md)
- Progress: [docs/ROADMAP.md](docs/ROADMAP.md) · Journals: [milestones/](milestones/)

## Requirements

Linux (on Windows: WSL2 Ubuntu 24.04), clang, CMake >= 3.25, Ninja, Python 3.

```sh
sudo apt install clang clang-tidy cmake ninja-build python3-venv
python3 -m venv .venv && .venv/bin/pip install -r bench/requirements.txt  # matplotlib, clang-format
```

GoogleTest, Google Benchmark, toml++, HdrHistogram_c and CLI11 are downloaded by CMake.

## Build and test

| Preset | Use |
| --- | --- |
| `debug` | day-to-day development |
| `asan` | AddressSanitizer (memory errors, leaks) |
| `tsan` | ThreadSanitizer (data races) |
| `release` | benchmarks |

```sh
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```

Output goes to `build/<preset>/`, binaries to `build/<preset>/bin/`.

**TSan on newer kernels** (including WSL2): clang 18's TSan aborts with
"unexpected memory mapping" when `vm.mmap_rnd_bits` is 32. Lower it until the next reboot:

```sh
sudo sysctl -w vm.mmap_rnd_bits=28
```

## Benchmarks

```sh
cmake --preset release && cmake --build --preset release

# One run -> JSON (throughput, p50/p95/p99 latency, config, git commit, machine)
build/release/bin/minitub_bench --workload=dummy --threads=4 --seconds=2 \
    --seed=42 --config=configs/default.toml --out=r.json

# Matrix of configs x threads, 5 repeats each, median kept; then a PNG chart
.venv/bin/python bench/sweep.py --workload dummy --threads 1 2 4 8 \
    --configs configs/default.toml --seconds 2 --repeats 5 --out-dir results/dummy
.venv/bin/python bench/plot.py results/dummy/*.json -o results/dummy.png

# Google Benchmark micro-benchmarks
build/release/bin/minitub_micro
```

Engine options live in TOML (`configs/default.toml` lists every key and its default).

## Code style

```sh
git ls-files '*.h' '*.cpp' | xargs .venv/bin/clang-format --dry-run --Werror   # check
git ls-files '*.h' '*.cpp' | xargs .venv/bin/clang-format -i                   # fix
cmake --preset debug -DMINITUB_CLANG_TIDY=ON                                  # clang-tidy while compiling
```

CI (GitHub Actions) checks formatting and runs the debug, asan and tsan presets on every push.
