#!/usr/bin/env python3
"""Plot sweep results: throughput and p99 latency vs threads, one line per series.

Example:
  python3 bench/plot.py results/dummy/*.json -o results/dummy.png

A series is "<workload> / <config name>". Throughput and latency get separate
panels (never two y-scales on one chart). The shaded band is the min..max
throughput across repeats.
"""
import argparse
import json
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # file output only, no display needed
import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter

# Validated categorical order (fixed, never cycled); markers are a second cue besides color.
COLORS = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#4a3aa7", "#e34948"]
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]
INK, INK_2, MUTED, GRID, SURFACE = "#0b0b0b", "#52514e", "#898781", "#e6e5e0", "#fcfcfb"


def load(paths):
    series = defaultdict(list)
    for path in paths:
        r = json.loads(Path(path).read_text())
        name = r.get("sweep", {}).get("config_name", "default")
        series[f"{r['workload']} / {name}"].append(r)
    for runs in series.values():
        runs.sort(key=lambda r: r["params"]["threads"])
    if len(series) > len(COLORS):
        raise SystemExit(f"{len(series)} series; at most {len(COLORS)} per chart (split into several charts)")
    return series


def style(ax, title, ylabel):
    ax.set_facecolor(SURFACE)
    ax.set_title(title, loc="left", color=INK, fontsize=11, fontweight="bold")
    ax.set_xlabel("threads", color=INK_2)
    ax.set_ylabel(ylabel, color=INK_2)
    ax.grid(axis="y", color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    ax.tick_params(colors=MUTED)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)
    ax.set_ylim(bottom=0)


def direct_labels(ax, points, min_gap_px=12):
    """Label each line at its last point, nudging labels apart so they never overlap."""
    to_px = ax.transData.transform
    placed = sorted((to_px((x, y))[1], x, y, label) for x, y, label in points)
    last_px = None
    for y_px, x, y, label in placed:
        target = y_px if last_px is None else max(y_px, last_px + min_gap_px)
        last_px = target
        ax.annotate(label, (x, y), xytext=(8, (target - y_px) * 72 / ax.figure.dpi),
                    textcoords="offset points", va="center", fontsize=8, color=INK_2)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("results", nargs="+", help="median-run JSON files from sweep.py")
    p.add_argument("-o", "--out", default="bench.png")
    args = p.parse_args()

    series = load(args.results)
    fig, (tput_ax, lat_ax) = plt.subplots(1, 2, figsize=(11, 4.2), facecolor=SURFACE)
    ends = {tput_ax: [], lat_ax: []}  # (x, y, label) of each line's last point

    for i, (label, runs) in enumerate(series.items()):
        threads = [r["params"]["threads"] for r in runs]
        tput = [r["results"]["throughput_ops_per_sec"] for r in runs]
        p99 = [r["results"]["latency_ns"]["p99"] for r in runs]
        kw = dict(color=COLORS[i], marker=MARKERS[i], markersize=6, linewidth=2, label=label)
        tput_ax.plot(threads, tput, **kw)
        if all("sweep" in r for r in runs):
            tput_ax.fill_between(threads, [r["sweep"]["throughput_min"] for r in runs],
                                 [r["sweep"]["throughput_max"] for r in runs], color=COLORS[i], alpha=0.15, linewidth=0)
        lat_ax.plot(threads, p99, **kw)
        ends[tput_ax].append((threads[-1], tput[-1], label))
        ends[lat_ax].append((threads[-1], p99[-1], label))
        print(f"{label}")
        for t, x, l in zip(threads, tput, p99):
            print(f"  threads={t:<3} throughput={x:>14,.0f} ops/s  p99={l:>8,} ns")

    style(tput_ax, "Throughput", "ops/s")
    style(lat_ax, "p99 latency", "ns")
    tput_ax.yaxis.set_major_formatter(EngFormatter())
    for ax in (tput_ax, lat_ax):
        ax.set_xticks(sorted({r["params"]["threads"] for runs in series.values() for r in runs}))
        ax.margins(x=0.15)
    if len(series) >= 2:
        tput_ax.legend(frameon=False, fontsize=8, labelcolor=INK_2)
    if len(series) <= 4:
        for ax, points in ends.items():
            direct_labels(ax, points)

    first = next(iter(series.values()))[0]
    b, m = first["build"], first["machine"]
    repeats = first.get("sweep", {}).get("repeats", 1)
    fig.suptitle(f"{m['cpu_model']} · {m['logical_cores']} threads · {b['build_type']} · "
                 f"commit {b['git_commit'][:7]}{' (dirty)' if b['git_dirty'] else ''} · "
                 f"median of {repeats} runs · {first['params']['seconds']}s each",
                 x=0.01, ha="left", fontsize=8, color=MUTED)
    fig.tight_layout()
    fig.savefig(args.out, dpi=150, facecolor=SURFACE)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
