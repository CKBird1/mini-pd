#!/usr/bin/env python3
"""Convert a .bench into Bookshelf v1 (.aux/.nodes/.nets/.scl) next to it."""

from __future__ import annotations

import argparse
from pathlib import Path


def fmt_num(x: float) -> str:
    r = round(x)
    if abs(x - r) < 1e-12:
        return str(int(r))
    return repr(x)


def parse_bench(path: Path):
    die = None
    rows = None
    cells = []
    nets = []
    for lineno, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        kw = parts[0]
        if kw == "DIE":
            die = tuple(float(x) for x in parts[1:5])
        elif kw == "ROWS":
            rows = (int(parts[1]), float(parts[2]))
        elif kw == "CELL":
            cells.append((parts[1], float(parts[2]), float(parts[3])))
        elif kw == "NET":
            nets.append((parts[1], parts[2:]))
        else:
            raise SystemExit(f"{path}:{lineno}: unknown keyword {kw}")
    if die is None or rows is None:
        raise SystemExit(f"{path}: missing DIE or ROWS")
    return die, rows, cells, nets


def write_bookshelf(bench: Path, die, rows, cells, nets) -> None:
    stem = bench.stem
    aux_dir = bench.parent
    x0, y0, x1, y1 = die
    nrows, row_h = rows
    site_w = 1.0
    num_sites = int(round((x1 - x0) / site_w))
    if abs(num_sites * site_w - (x1 - x0)) > 1e-9:
        raise SystemExit(f"{bench}: die width is not an integer number of sites")

    nodes_name = f"{stem}.nodes"
    nets_name = f"{stem}.nets"
    scl_name = f"{stem}.scl"

    nodes = [
        "UCLA nodes 1.0",
        f"# Same netlist as {bench.name}",
        f"NumNodes : {len(cells)}",
        "NumTerminals : 0",
    ]
    for name, w, h in cells:
        nodes.append(f"{name} {fmt_num(w)} {fmt_num(h)}")
    (aux_dir / nodes_name).write_text("\n".join(nodes) + "\n")

    n_pins = sum(len(pins) for _, pins in nets)
    net_lines = [
        "UCLA nets 1.0",
        f"NumNets : {len(nets)}",
        f"NumPins : {n_pins}",
    ]
    for name, pins in nets:
        net_lines.append(f"NetDegree : {len(pins)} {name}")
        for cell in pins:
            net_lines.append(f"  {cell} B : 0.0 0.0")
    (aux_dir / nets_name).write_text("\n".join(net_lines) + "\n")

    scl = [
        "UCLA scl 1.0",
        f"NumRows : {nrows}",
        "",
    ]
    for i in range(nrows):
        y = y0 + i * row_h
        scl.extend(
            [
                "CoreRow Horizontal",
                f"  Coordinate    : {fmt_num(y)}",
                f"  Height        : {fmt_num(row_h)}",
                f"  Sitewidth     : {fmt_num(site_w)}",
                f"  Sitespacing   : {fmt_num(site_w)}",
                "  Siteorient    : N",
                "  Sitesymmetry  : Y",
                f"  SubrowOrigin  : {fmt_num(x0)}  NumSites : {num_sites}",
                "End",
            ]
        )
    (aux_dir / scl_name).write_text("\n".join(scl) + "\n")

    (aux_dir / f"{stem}.aux").write_text(
        f"RowBasedPlacement : {nodes_name} {nets_name} {scl_name}\n"
    )


def emit_large(path: Path) -> None:
    """200 cells, 400x200 die, 20 rows. Dense GE + Abacus stay interactive."""
    n = 200
    group = 10
    widths = [4, 4, 6, 4, 8, 4, 4, 6]
    lines = [
        "# 200 cells on a 400x200 die (20 rows). ~10x medium; dense quadratic",
        "# and later G-cell GR stay interactive.",
        "DIE 0 0 400 200",
        "ROWS 20 10",
    ]
    for i in range(n):
        lines.append(f"CELL c{i} {widths[i % len(widths)]} 2")
    # Local 2-pin chain inside each group of 10, plus a 4-pin span.
    for g in range(n // group):
        b = g * group
        for i in range(group - 1):
            lines.append(f"NET n{b + i} c{b + i} c{b + i + 1}")
        lines.append(f"NET q{g} c{b} c{b + 3} c{b + 6} c{b + 9}")
    for g in range(n // group - 1):
        a = g * group + (group - 1)
        b = (g + 1) * group
        lines.append(f"NET x{g} c{a} c{b}")
    clk = " ".join(f"c{i}" for i in range(0, n, group))
    lines.append(f"NET clk {clk}")
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "benches",
        nargs="*",
        type=Path,
        help=".bench files to convert (default: data/medium.bench data/large.bench)",
    )
    ap.add_argument(
        "--emit-large",
        action="store_true",
        help="rewrite data/large.bench from the recipe, then convert",
    )
    args = ap.parse_args()
    root = Path(__file__).resolve().parents[1]
    data = root / "data"
    if args.emit_large:
        emit_large(data / "large.bench")
    benches = args.benches
    if not benches:
        benches = [data / "medium.bench", data / "large.bench"]
    for b in benches:
        die, rows, cells, nets = parse_bench(b)
        write_bookshelf(b, die, rows, cells, nets)
        print(f"wrote {b.stem}.aux/.nodes/.nets/.scl ({len(cells)} cells, {len(nets)} nets)")


if __name__ == "__main__":
    main()
