#include "metrics/hpwl.hpp"

namespace minipd {

double hpwl(const Design& design) {
    if(design.nets.empty()) {
        return 0.0;
    }

    double total = 0.0;

    for(const auto& n : design.nets) {
        if(n.pins.size() < 2) continue;
        double x0 = 0.0, x1 = 0.0, y0 = 0.0, y1 = 0.0;
        x0 = x1 = design.cells[n.pins[0]].x;
        y0 = y1 = design.cells[n.pins[0]].y;
        for(auto p : n.pins) {
            if(design.cells[p].x < x0) x0 = design.cells[p].x;
            if(design.cells[p].x > x1) x1 = design.cells[p].x;
            if(design.cells[p].y < y0) y0 = design.cells[p].y;
            if(design.cells[p].y > y1) y1 = design.cells[p].y;
        } 
        total += ((x1 - x0) + (y1 - y0));  
    }
    
    return total;
}

}  // namespace minipd
