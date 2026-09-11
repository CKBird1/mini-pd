# mini-pd

A small C++ physical-design engine: netlist in, global place, legalize, global route, then metrics and an SVG.

The CLI and IR are meant to stay stable while the algorithms improve. Quality of the first algorithms is 'easiest' to get a working flow. This repo is a learning engine with much room for algorithmic improvements as I go. G-cell router (L + MST) is in and benched; next is a deeper synthesis project.

## Status

Day 1: (2026-09-07).    Skeleton done, testing method done, mini-parser done, small benchmark designs done, hpwl and random done, no snap yet

Day 2: (2026-09-08).    Added snap. Rule is snap to nearest legal row (round not floor) and then pack left with no extra space.
                        Added quadratic, better than random, although with snap only for legalize the results don't look as impressive as they should.
                        Added abacus tetris, better HPWL than snap (310.8 vs 336 snap on my medium bench)

Day 3: (2026-09-09).    Added full clusters for Abacus. Improves over tetris abacus from yesterday: 306.3 vs 310.8
                        Added bookshelf functionality for reading in other peoples designs.

Day 4: (2026-09-10).    Added grid object to track global router with basic functionality. Added 2-pin L shape usage check and 
                        usage update. No MST yet

Day 5: (2026-09-11).    Added final 3+ pin MST using Kruskals, then L connection on the pins
                        Updated this readme to include some final data, QoR, testing and thoughts. Good to wrap up a project with a post-mortem before moving on. If I come back eventually, I'd be adding multiple more placer, legalizer and router algorithms and then some smart-decision making on which to select per design. I would also make sure to account for 
                        congestion, choosing grid edges that get overflow to have higher weights to make them selected less often. Common
                        practice for this type of work.

QoR Results:
large.bench, 10-seed mean

| path             | HPWL  | legal   | ov  | gcell_wl |
| ---------------- | ----- | ------- | --- | -------- |
| quadratic+abacus | 12277 | 200/200 | 66  | 1265     |
| quadratic+snap   | 20703 | 98/200  | 397 | 913      |
| random+snap      | 20976 | 200/200 | 459 | 2133     |
| random+abacus    | 47201 | 200/200 | 0   | 4884     |

The default flow state for the project is quadratic placement -> abacus legalizer -> MST Routing using Kruskals. Random placement -> snap legalizer was the simple 'first steps' for placer/legalizer to get a working flow, and then the other algorithms used to show obvious improvement, using intelligence and consideration of extra factors to get a better starting point and a better manipulation of that through legalizer. Finally the router builds a Kruskal MST on pin G-cells (n-1 tree edges per net) and L-patterns each edge. It reports congestion after the fact (overflow / max_overflow). Same-G-cell pins union with no wire, so we do not paint grid edges unnecessarily.

The above data shows only the large bench design as it is the most resembling of a real design rather than tiny, medium, and medium_2pin. medium_2pin is specifically for testing 2-pin L-shape connections only, rather than trying to resemble a real design. Using Kruskals we can take a group of pins that make up a single net and choose the pins-1 paths to minimize path length. Currently no extra weight is given to highly used paths.

HPWL is the half-perimeter of each net's pin bbox summed together. This shows an overall placement closeness compared to other methods.
legal x/y is how many cells are legal vs how many cells exist in the design. Quadratic + Snap shows an example where despite having a great gcell_wl (grid cell edge usage) it only achieves that by having over half of the cells illegal. 
OV is total extra demand for the entire design, meaning how many times a path is used over its max capacity. There is another point of data, max_overflow which says how much over capacity the hottest path is.
gcell_wl is grid cell edge usage as mentioned above, its essentially saying how many 1-cell length paths are used throughout each design. The closer the cells are to where the nets need to go, the shorter the paths and therefore the lower the gcell_wl usage will be.

From the chart quadratic + abacus creates the best set of results all things considered: 100% legal placement, smallest HPWL so the cells are close to each other, and the best gcell edge usage that is still legal. random+abacus has overflow 0 only because the cells are spread (HPWL ~4x worse). Among short legal paths, quadratic+abacus is the leftover overflow (L does not detour). Snap after random matches quadratic+snap HPWL because left-pack collapses to x=0, not because snap is a better legalizer.


## Tiny netlist format

```
DIE 0 0 100 100
ROWS 10 10
CELL c1 4 2
CELL c2 4 2
NET n1 c1 c2
```

| Record | Meaning |
| --- | --- |
| `DIE x0 y0 x1 y1` | Die bbox. Layout coords: x right, y up, origin lower-left. |
| `ROWS <count> <height>` | `count` horizontal rows stacked up from `die.y0`. Leftover die height is allowed. |
| `CELL name width height` | Movable instance. No initial (x, y); the placer assigns it. |
| `NET name cell cell ...` | Hyperedge. Phase 0 pin is the cell lower-left. |

`#` starts a comment.

Benches: `data/tiny.bench`, `data/medium.bench`, `data/large.bench`.
`data/medium_2pin.bench` is a 2-pin L test, not placer gold.
Bookshelf twins: `data/*.aux` (plus `.nodes` / `.nets` / `.scl`).

## Build (Linux)

Needs CMake 3.16+, a C++17 compiler (g++ 9+ or clang 9+).
```bash
./build.sh
./build/mini-pd data/tiny.bench -o out/
./build/test_hpwl
./build/test_overflow
./build/test_mst
./scripts/sweep.py --seeds 1 --benches data/tiny.bench   # smoke
./scripts/sweep.py -o out/sweep.csv                      # 10 seeds × 4 paths × 4 benches
```

## CLI

```
./build/mini-pd <input.bench|.aux> -o <outdir> [--seed N] [--placer quadratic|random] [--legalizer abacus|snap]
```

Writes `<outdir>/placed.svg` and `<outdir>/qor.txt`. Default seed is 1. Default placer is `quadratic`. Default legalizer is `abacus`. Pass `--placer random` and/or `--legalizer snap` to use the baselines. Always runs the G-cell global router.

## Layout

```
src/ir/          design IR
src/io/          tiny-format parser, Bookshelf (.aux)
src/place/       IPlacer — Random, quadratic
src/legalize/    ILegalizer — Snap, Abacus
src/metrics/     HPWL
src/viz/         SVG + qor.txt (hpwl + overflow)
src/route/       IRouter — G-cell GR (grid + overflow; MST + L in)
src/flow.cpp     CLI
scripts/         .bench → Bookshelf twins
tests/           HPWL + overflow + MST contract tests
```

## Roadmap

0. Basic skeleton, functional flow, testing system, output files/QoR results. Snap, Random, and HPWL for basic flow
1. Bookshelf + quadratic/force placer + Abacus.
2. G-cell router (2-pin L + 3+ pin MST) 
3. Sweep scripts, tech note, compare to old algorithms, Done

