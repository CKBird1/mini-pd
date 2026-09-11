#!/usr/bin/env python3
"""Run mini-pd over benches × placers × legalizers × seeds. Writes CSV.

Each row is one run. A mean-over-seeds table is printed to stderr at the end.

  ./scripts/sweep.py -o out/sweep.csv
  ./scripts/sweep.py --seeds 1 --benches data/tiny.bench data/medium.bench
"""

from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

QOR_KEYS = (
    "cells",
    "legal",
    "hpwl",
    "overflow",
    "max_overflow",
    "gcell_wl",
)

INT_KEYS = {"cells", "legal", "overflow", "max_overflow", "gcell_wl"}

PATHS = (
    ("quadratic", "abacus"),
    ("quadratic", "snap"),
    ("random", "abacus"),
    ("random", "snap"),
)

DEFAULT_BENCHES = (
    "data/tiny.bench",
    "data/medium.bench",
    "data/medium_2pin.bench",
    "data/large.bench",
)


def parse_seeds(spec: str) -> list[int]:
    seeds: list[int] = []
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            a, b = part.split("-", 1)
            lo, hi = int(a), int(b)
            if hi < lo:
                raise SystemExit(f"bad seed range: {part}")
            seeds.extend(range(lo, hi + 1))
        else:
            seeds.append(int(part))
    if not seeds:
        raise SystemExit("no seeds")
    return seeds


def parse_qor(path: Path) -> dict:
    got = {}
    for line in path.read_text().splitlines():
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        got[k.strip()] = v.strip()
    row = {}
    for k in QOR_KEYS:
        if k not in got:
            raise SystemExit(f"{path}: missing {k}")
        raw = got[k].split()[0]
        row[k] = int(raw) if k in INT_KEYS else float(raw)
    return row


def mean_table(rows: list[dict]) -> None:
    buckets: dict[tuple, list[dict]] = defaultdict(list)
    for r in rows:
        buckets[(r["input"], r["placer"], r["legalizer"])].append(r)

    fields = ("hpwl", "legal", "overflow", "max_overflow", "gcell_wl")
    header = f"{'input':<24} {'path':<18} {'n':>3} {'hpwl':>12} {'legal':>8} {'ov':>8} {'max_ov':>8} {'gcell_wl':>10}"
    print(header, file=sys.stderr)
    print("-" * len(header), file=sys.stderr)
    for key in sorted(buckets):
        group = buckets[key]
        n = len(group)
        means = {f: sum(g[f] for g in group) / n for f in fields}
        path = f"{key[1]}+{key[2]}"
        print(
            f"{key[0]:<24} {path:<18} {n:>3} "
            f"{means['hpwl']:>12.3f} {means['legal']:>8.1f} "
            f"{means['overflow']:>8.1f} {means['max_overflow']:>8.1f} "
            f"{means['gcell_wl']:>10.1f}",
            file=sys.stderr,
        )


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--bin",
        default=str(repo / "build" / "mini-pd"),
        help="path to mini-pd (default: ./build/mini-pd)",
    )
    p.add_argument(
        "--benches",
        nargs="+",
        default=[str(repo / b) for b in DEFAULT_BENCHES],
        help="input .bench/.aux files",
    )
    p.add_argument(
        "--seeds",
        default="1-10",
        help="comma list and/or range, e.g. 1-10 or 1,2,5 (default: 1-10)",
    )
    p.add_argument(
        "-o",
        "--output",
        default="-",
        help="CSV path, or - for stdout (default: -)",
    )
    args = p.parse_args()

    binary = Path(args.bin)
    if not binary.is_file():
        raise SystemExit(f"missing binary: {binary} (run ./build.sh)")

    seeds = parse_seeds(args.seeds)
    benches = [Path(b) for b in args.benches]
    for b in benches:
        if not b.is_file():
            raise SystemExit(f"missing bench: {b}")

    jobs = [
        (bench, placer, legalizer, seed)
        for bench in benches
        for placer, legalizer in PATHS
        for seed in seeds
    ]

    out_fp = sys.stdout if args.output == "-" else open(args.output, "w", newline="")
    writer = csv.DictWriter(
        out_fp,
        fieldnames=["input", "placer", "legalizer", "seed", *QOR_KEYS],
    )
    writer.writeheader()
    if args.output != "-":
        out_fp.flush()

    rows: list[dict] = []
    try:
        with tempfile.TemporaryDirectory(prefix="minipd-sweep-") as tmp:
            tmp_path = Path(tmp)
            for i, (bench, placer, legalizer, seed) in enumerate(jobs, 1):
                tag = f"{bench.stem}_{placer}_{legalizer}_{seed}"
                print(
                    f"[{i}/{len(jobs)}] {bench.name} {placer}+{legalizer} seed={seed}",
                    file=sys.stderr,
                )
                outdir = tmp_path / tag
                outdir.mkdir()
                cmd = [
                    str(binary),
                    str(bench),
                    "-o",
                    str(outdir),
                    "--seed",
                    str(seed),
                    "--placer",
                    placer,
                    "--legalizer",
                    legalizer,
                ]
                r = subprocess.run(cmd, capture_output=True, text=True)
                if r.returncode != 0:
                    sys.stderr.write(r.stderr)
                    raise SystemExit(f"run failed ({r.returncode}): {' '.join(cmd)}")
                qor = parse_qor(outdir / "qor.txt")
                row = {
                    "input": str(bench.as_posix() if bench.is_absolute() else bench),
                    "placer": placer,
                    "legalizer": legalizer,
                    "seed": seed,
                    **qor,
                }
                # Prefer a stable relative path in the CSV when possible.
                try:
                    row["input"] = str(bench.resolve().relative_to(repo))
                except ValueError:
                    row["input"] = str(bench)
                writer.writerow(row)
                out_fp.flush()
                rows.append(row)
    finally:
        if args.output != "-":
            out_fp.close()

    print(file=sys.stderr)
    print("mean over seeds", file=sys.stderr)
    mean_table(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
