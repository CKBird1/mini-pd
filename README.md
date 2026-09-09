# mini-pd

A small C++ physical-design engine: netlist in, global place, legalize, then metrics and an SVG.

The CLI and IR are meant to stay stable while the algorithms improve. Quality of the first algorithms is 'easiest' to get a working flow. This repo is a learning engine with much room for algorithmic improvements as I go. Eventually I'd like to add some form of router, and maybe even synthesis.

## Status

Day 1: (2026-09-07).    Skeleton done, testing method done, mini-parser done, small benchmark designs done, hpwl and random done, no snap yet
Day 2: (2026-09-08).    Added snap. Rule is snap to nearest legal row (round not floor) and then pack left with no extra space.
                        Added quadratic, better than random, although with snap only for legalize the results don't look as impressive as they should.


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

Benches: `data/tiny.bench`, `data/medium.bench`.

## Build (Linux)

Needs CMake 3.16+, a C++17 compiler (g++ 9+ or clang 9+).
```bash
./build.sh
./build/mini-pd data/tiny.bench -o out/
./build/test_hpwl
```

## CLI

```
./build/mini-pd <input.bench> -o <outdir> [--seed N] [--placer random|quadratic]
```

Writes `<outdir>/placed.svg` and `<outdir>/qor.txt`. Default seed is 1. Default placer is `random`.

## Layout

```
src/ir/          design IR
src/io/          tiny-format parser
src/place/       IPlacer — Random, quadratic
src/legalize/    ILegalizer — Snap first
src/metrics/     HPWL
src/viz/         SVG + qor.txt
src/route/       header only; not started
src/flow.cpp     CLI
tests/           HPWL contract test
```

## Roadmap

0. Basic skeleton, functional flow, testing system, output files/QoR results. Snap, Random, and HPWL for basic flow
1. Bookshelf + quadratic/force placer + Abacus.
2. Congestion / optional unit-delay slack force + one perf pass.
3. Sweep scripts, tech note, compare to other engines/old algorithms
4. Synthesis on the same IR.
5. G-cell router and beyond
