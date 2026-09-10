#include "ir/design.hpp"
#include "route/gcell.hpp"

#include <iostream>

int main() {
    minipd::Design d;
    d.die.bbox = {0, 0, 20, 10};
    minipd::Row row;
    row.y = 0;
    row.height = 10;
    row.x0 = 0;
    row.x1 = 20;
    d.rows.push_back(row);

    minipd::GcellGrid g;
    g.build(d);

    // 20x10 die, tile 10 → 2x1 G-cells, one H edge, cap 10.
    if (g.nx != 2 || g.ny != 1 || g.capacity != 10 || g.h_use.size() != 1 ||
        g.v_use.size() != 0) {
        std::cerr << "grid: nx=" << g.nx << " ny=" << g.ny
                  << " cap=" << g.capacity << " h=" << g.h_use.size()
                  << " v=" << g.v_use.size() << "\n";
        return 1;
    }

    minipd::GcellCoord a = g.pin_gcell(0, 0);
    minipd::GcellCoord b = g.pin_gcell(15, 0);
    if (a.ix != 0 || a.iy != 0 || b.ix != 1 || b.iy != 0) {
        std::cerr << "pin map: (" << a.ix << "," << a.iy << ") (" << b.ix << ","
                  << b.iy << ")\n";
        return 1;
    }

    if (g.overflow() != 0 || g.max_overflow() != 0 || g.wirelength() != 0) {
        std::cerr << "empty grid should be zero overflow\n";
        return 1;
    }

    g.add_h(0, 0);
    g.add_h(0, 0);
    if (g.overflow() != 0 || g.wirelength() != 2) {
        std::cerr << "under cap: ov=" << g.overflow() << " wl=" << g.wirelength()
                  << "\n";
        return 1;
    }

    g.h_use[0] = 12;
    if (g.overflow() != 2 || g.max_overflow() != 2 || g.wirelength() != 12) {
        std::cerr << "overflow: ov=" << g.overflow() << " max=" << g.max_overflow()
                  << " wl=" << g.wirelength() << "\n";
        return 1;
    }

    std::cout << "PASS\n";
    return 0;
}
