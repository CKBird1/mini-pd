#include "route/gcell.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace minipd {

void GcellGrid::build(const Design& design) {
    const Rect& die = design.die.bbox;
    if (die.width() <= 0 || die.height() <= 0) {
        throw std::runtime_error("cannot route empty die");
    }
    if (design.rows.empty() || design.rows.front().height <= 0) {
        throw std::runtime_error("cannot route without rows");
    }

    x0 = die.x0;
    y0 = die.y0;
    gh = design.rows.front().height;
    gw = gh;
    nx = std::max(1, static_cast<int>(std::ceil(die.width() / gw)));
    ny = std::max(1, static_cast<int>(std::ceil(die.height() / gh)));
    capacity = std::max(1, static_cast<int>(std::lround(gw)));

    const int nh = ny * std::max(0, nx - 1);
    const int nv = std::max(0, ny - 1) * nx;
    h_cap.assign(static_cast<std::size_t>(nh), capacity);
    h_use.assign(static_cast<std::size_t>(nh), 0);
    v_cap.assign(static_cast<std::size_t>(nv), capacity);
    v_use.assign(static_cast<std::size_t>(nv), 0);
}

GcellCoord GcellGrid::pin_gcell(double x, double y) const {
    if (nx <= 0 || ny <= 0 || gw <= 0 || gh <= 0) {
        throw std::runtime_error("G-cell grid is empty");
    }
    int ix = static_cast<int>(std::floor((x - x0) / gw));
    int iy = static_cast<int>(std::floor((y - y0) / gh));
    ix = std::max(0, std::min(ix, nx - 1));
    iy = std::max(0, std::min(iy, ny - 1));
    return {ix, iy};
}

int GcellGrid::h_index(int ix, int iy) const {
    if (ix < 0 || ix >= nx - 1 || iy < 0 || iy >= ny) {
        throw std::runtime_error("h-edge out of grid");
    }
    return iy * (nx - 1) + ix;
}

int GcellGrid::v_index(int ix, int iy) const {
    if (ix < 0 || ix >= nx || iy < 0 || iy >= ny - 1) {
        throw std::runtime_error("v-edge out of grid");
    }
    return iy * nx + ix;
}

void GcellGrid::add_h(int ix, int iy) {
    ++h_use[static_cast<std::size_t>(h_index(ix, iy))];
}

void GcellGrid::add_v(int ix, int iy) {
    ++v_use[static_cast<std::size_t>(v_index(ix, iy))];
}

int GcellGrid::hv_usage(int iix, int iiy, int jix, int jiy, bool also_add) {
    //Assume all incoming points are valid (iix, iiy), (jix, jiy)
    //Track horiz first
    int h_usage = 0;
    if(iix != jix) {
        for(int i = std::min(iix, jix); i < std::max(iix, jix); ++i) {
            h_usage += h_use[h_index(i, iiy)];
            if(also_add) add_h(i, iiy);
        }
    }

    int prev_x = jix;
    int v_usage = 0;
    if(iiy != jiy) {
        for(int i = std::min(iiy, jiy); i < std::max(iiy, jiy); ++i) {
            v_usage += v_use[v_index(prev_x, i)];
            if(also_add) add_v(prev_x, i);
        }
    }
    
    return h_usage + v_usage;
}

int GcellGrid::vh_usage(int iix, int iiy, int jix, int jiy, bool also_add) {
    int v_usage = 0;
    if(iiy != jiy) {
        for(int i = std::min(iiy, jiy); i < std::max(iiy, jiy); ++i) {
            v_usage += v_use[v_index(iix, i)];
            if(also_add) add_v(iix, i);
        }
    }

    int prev_y = jiy;
    int h_usage = 0;
    if(iix != jix) { //We have to move horizontally at least one space
        for(int i = std::min(iix, jix); i < std::max(iix, jix); ++i) {
            h_usage += h_use[h_index(i, prev_y)];
            if(also_add) add_h(i, prev_y);
        }
    }
    
    return h_usage + v_usage;
}

int GcellGrid::overflow() const {
    int sum = 0;
    for (std::size_t i = 0; i < h_use.size(); ++i) {
        if (h_use[i] > h_cap[i]) {
            sum += h_use[i] - h_cap[i];
        }
    }
    for (std::size_t i = 0; i < v_use.size(); ++i) {
        if (v_use[i] > v_cap[i]) {
            sum += v_use[i] - v_cap[i];
        }
    }
    return sum;
}

int GcellGrid::max_overflow() const {
    int m = 0;
    for (std::size_t i = 0; i < h_use.size(); ++i) {
        m = std::max(m, h_use[i] - h_cap[i]);
    }
    for (std::size_t i = 0; i < v_use.size(); ++i) {
        m = std::max(m, v_use[i] - v_cap[i]);
    }
    return m;
}

int GcellGrid::wirelength() const {
    int sum = 0;
    for (int u : h_use) {
        sum += u;
    }
    for (int u : v_use) {
        sum += u;
    }
    return sum;
}

const char* GcellRouter::name() const {
    return "gcell";
}

void GcellRouter::route(Design& design) {
    grid_.build(design);
    // MST of pin G-cells, then L-pattern each 2-pin.
    for(const auto& n : design.nets) {
        if(n.pins.size() < 2) continue;
        if(n.pins.size() > 2) continue; //Skip these for now, come back later, about to start implementation for MST which will account for 3+

        //Guaranteed 2 pins on this net, check if same gcell
        std::vector<GcellCoord> c_pins;
        for(const auto& p : n.pins) {
            c_pins.push_back(grid_.pin_gcell(design.cells[p].x, design.cells[p].y));
        }

        //Now have a vector of all pins (currently should be size 2, updating next)
        int x = c_pins[0].ix;
        int y = c_pins[0].iy;
        bool same_cell = true;
        for(size_t i = 1; i < c_pins.size(); ++i) {
            if(c_pins[i].ix != x) same_cell = false;
            if(c_pins[i].iy != y) same_cell = false;
        }
        if(same_cell) continue;

        //Now we can create the actual L shape
        int hvUsage = grid_.hv_usage(c_pins[0].ix, c_pins[0].iy, c_pins[1].ix, c_pins[1].iy, false);
        int vhUsage = grid_.vh_usage(c_pins[0].ix, c_pins[0].iy, c_pins[1].ix, c_pins[1].iy, false);

        if(hvUsage <= vhUsage) (void)grid_.hv_usage(c_pins[0].ix, c_pins[0].iy, c_pins[1].ix, c_pins[1].iy, true);
        else (void)grid_.vh_usage(c_pins[0].ix, c_pins[0].iy, c_pins[1].ix, c_pins[1].iy, true);
    }
}

}  // namespace minipd
