#include "route/gcell.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

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
        
        std::vector<GcellCoord> c_pins;
        std::vector<int> pin_parents(n.pins.size(), 0);
        for(std::size_t p = 0; p < n.pins.size(); ++p) {
            c_pins.push_back(grid_.pin_gcell(design.cells[n.pins[p]].x, design.cells[n.pins[p]].y));
            pin_parents[p] = p;
        }

        //Now have a vector of all pins, first we need to create a vector of tuples, and then sort by smallest manhattan distance
        std::vector<std::tuple<int, std::size_t, std::size_t>> possible_connections;
        for(std::size_t i = 0; i < c_pins.size(); ++i) {
            for(std::size_t j = i+1; j < c_pins.size(); ++j) {
                //This is now every pair exactly once, find the manhattan and store
                int manhat = (abs(c_pins[i].ix - c_pins[j].ix)) + (abs(c_pins[i].iy - c_pins[j].iy));
                possible_connections.push_back(std::tuple(manhat, i, j));
            }
        }

        //Now sort by manhat
        std::sort(possible_connections.begin(), possible_connections.end()); 

        auto find = [&](std::size_t x) {
            while(pin_parents[x] != static_cast<int>(x)) {
                x = static_cast<std::size_t>(pin_parents[x]);
            }
            return x;
        };

        //Now go through sorted vector and start making pairs
        for(std::size_t i = 0; i < possible_connections.size(); ++i) {
            //First make sure they aren't already in the same group
            if(find(std::get<1>(possible_connections[i])) == find(std::get<2>(possible_connections[i]))) continue;

            //Next make sure the cells aren't the same physical gcell
            bool same_cell =    (c_pins[std::get<1>(possible_connections[i])].ix == c_pins[std::get<2>(possible_connections[i])].ix) && 
                                (c_pins[std::get<1>(possible_connections[i])].iy == c_pins[std::get<2>(possible_connections[i])].iy);
            if(same_cell) { //Group together and don't make a path
                pin_parents[find(std::get<2>(possible_connections[i]))] = pin_parents[find(std::get<1>(possible_connections[i]))];
                continue;
            }

            //We've decided to keep this pair and update everything
            //Then use prev 2-pin alg to choose the best path, and submit it
            int hvUsage = grid_.hv_usage(c_pins[std::get<1>(possible_connections[i])].ix, c_pins[std::get<1>(possible_connections[i])].iy, c_pins[std::get<2>(possible_connections[i])].ix, c_pins[std::get<2>(possible_connections[i])].iy, false);
            int vhUsage = grid_.vh_usage(c_pins[std::get<1>(possible_connections[i])].ix, c_pins[std::get<1>(possible_connections[i])].iy, c_pins[std::get<2>(possible_connections[i])].ix, c_pins[std::get<2>(possible_connections[i])].iy, false);

            if(hvUsage <= vhUsage) (void)grid_.hv_usage(c_pins[std::get<1>(possible_connections[i])].ix, c_pins[std::get<1>(possible_connections[i])].iy, c_pins[std::get<2>(possible_connections[i])].ix, c_pins[std::get<2>(possible_connections[i])].iy, true);
            else (void)grid_.vh_usage(c_pins[std::get<1>(possible_connections[i])].ix, c_pins[std::get<1>(possible_connections[i])].iy, c_pins[std::get<2>(possible_connections[i])].ix, c_pins[std::get<2>(possible_connections[i])].iy, true);

            //Update parent of both pins to the first pin's parent
            pin_parents[find(std::get<2>(possible_connections[i]))] = pin_parents[find(std::get<1>(possible_connections[i]))];
            
        }
    }
}

}  // namespace minipd
