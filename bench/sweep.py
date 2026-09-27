#!/usr/bin/env python3
"""Run minitub_bench over a matrix of configs x thread counts.

Every point runs --repeats times; the run with the median throughput is kept
(an odd repeat count keeps a real run, so its latencies stay consistent).

Example:
  python3 bench/sweep.py --workload dummy --threads 1 2 4 8 \
      --configs configs/default.toml --seconds 2 --repeats 5 --out-dir results/dummy

Writes <out-dir>/raw/*.json (every run) and <out-dir>/<config>-t<N>.json (median runs).
"""
import argparse
import json
import statistics
import subprocess
import sys
from pathlib import Path


def run_once(args, config, threads, out_file):
    cmd = [args.bin, f"--workload={args.workload}", f"--threads={threads}",
           f"--seconds={args.seconds}", f"--seed={args.seed}", f"--out={out_file}"]
    if config:
        cmd.append(f"--config={config}")
    subprocess.run(cmd, check=True, stderr=subprocess.DEVNULL)
    return json.loads(Path(out_file).read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--bin", default="build/release/bin/minitub_bench")
    p.add_argument("--workload", required=True)
    p.add_argument("--configs", nargs="+", default=["configs/default.toml"])
    p.add_argument("--threads", nargs="+", type=int, default=[1, 2, 4])
    p.add_argument("--seconds", type=float, default=2.0)
    p.add_argument("--repeats", type=int, default=5)
    p.add_argument("--seed", type=int, default=42)
    p.add_argument("--out-dir", required=True, type=Path)
    args = p.parse_args()

    raw_dir = args.out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)

    for config in args.configs:
        name = Path(config).stem
        for threads in args.threads:
            runs = [run_once(args, config, threads, raw_dir / f"{name}-t{threads}-r{i}.json")
                    for i in range(args.repeats)]
            tputs = [r["results"]["throughput_ops_per_sec"] for r in runs]
            median = runs[tputs.index(statistics.median_low(tputs))]
            median["sweep"] = {"config_name": name, "config_file": config, "repeats": args.repeats,
                               "throughput_all": tputs, "throughput_min": min(tputs), "throughput_max": max(tputs)}
            (args.out_dir / f"{name}-t{threads}.json").write_text(json.dumps(median, indent=2))
            print(f"{name:>16} t={threads:<3} median {median['results']['throughput_ops_per_sec']:>14,.0f} ops/s"
                  f"  (min {min(tputs):,.0f}, max {max(tputs):,.0f})", file=sys.stderr)


if __name__ == "__main__":
    main()
