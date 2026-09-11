#pragma once

#include "ir/design.hpp"
#include "route/router.hpp"

#include <vector>

namespace minipd {

// 2D G-cell global router. No layers, no vias, pin = cell lower-left.
//
// Grid
//   Square tiles. Side = first row height (same as a standard-cell row).
//   nx = ceil(die.width  / side), ny = ceil(die.height / side).
//   Pin (x, y) maps to (floor((x-x0)/side), floor((y-y0)/side)), clamped.
//
// Edges
//   H: (ix, iy) → (ix+1, iy)   ix in [0, nx-2], iy in [0, ny-1]
//   V: (ix, iy) → (ix, iy+1)   ix in [0, nx-1], iy in [0, ny-2]
//   Track pitch = 1, so capacity C = max(1, round(side)).
//   overflow     = sum max(0, usage - C)
//   max_overflow = max of those
//   gcell_wl     = sum of usage (G-cell hops)
//
// Kernel (GcellRouter::route):
//   MST on pin G-cells (Manhattan), then L-pattern each 2-pin.
//   Pick the L with less congestion. Maze / INFINITY cost later.
//
// Does not mutate Design. Usage lives on the grid.

struct GcellCoord {
    int ix = 0;
    int iy = 0;
};

struct GcellGrid {
    int nx = 0;
    int ny = 0;
    double x0 = 0;
    double y0 = 0;
    double gw = 0;
    double gh = 0;
    int capacity = 0;
    std::vector<int> h_cap;
    std::vector<int> h_use;
    std::vector<int> v_cap;
    std::vector<int> v_use;

    void build(const Design& design);

    GcellCoord pin_gcell(double x, double y) const;
    int h_index(int ix, int iy) const;
    int v_index(int ix, int iy) const;
    void add_h(int ix, int iy);
    void add_v(int ix, int iy);
    int hv_usage(int iix, int iiy, int jix, int jiy, bool also_add);
    int vh_usage(int iix, int iiy, int jix, int jiy, bool also_add);

    int overflow() const;
    int max_overflow() const;
    int wirelength() const;
};

class GcellRouter final : public IRouter {
public:
    void route(Design& design) override;
    const char* name() const override;

    const GcellGrid& grid() const { return grid_; }
    int overflow() const { return grid_.overflow(); }
    int max_overflow() const { return grid_.max_overflow(); }
    int wirelength() const { return grid_.wirelength(); }

private:
    GcellGrid grid_;
};

}  // namespace minipd
