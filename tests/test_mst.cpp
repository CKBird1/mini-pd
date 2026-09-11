#include "ir/design.hpp"
#include "route/gcell.hpp"

#include <iostream>
#include <vector>

// Pins at cell lower-left. G-cell side = row height.

static minipd::Cell make_cell(const char* name, double x, double y) {
    minipd::Cell c;
    c.name = name;
    c.width = 1;
    c.height = 1;
    c.x = x;
    c.y = y;
    c.placed = true;
    c.legal = true;
    return c;
}

static minipd::Design make_grid(double die_w, double die_h, double row_h) {
    minipd::Design d;
    d.die.bbox = {0, 0, die_w, die_h};
    for (double y = 0; y + 1e-9 < die_h; y += row_h) {
        minipd::Row r;
        r.y = y;
        r.height = row_h;
        r.x0 = 0;
        r.x1 = die_w;
        d.rows.push_back(r);
    }
    return d;
}

static void add_net(minipd::Design& d, const std::vector<std::size_t>& pins) {
    minipd::Net n;
    n.name = "n";
    n.pins = pins;
    d.nets.push_back(n);
}

static int route_wl(minipd::Design d, const char* label, int want) {
    minipd::GcellRouter r;
    r.route(d);
    const int got = r.wirelength();
    if (got != want) {
        std::cerr << "FAIL " << label << ": gcell_wl=" << got << " want=" << want
                  << " ov=" << r.overflow() << "\n";
        const auto& g = r.grid();
        std::cerr << "  grid " << g.nx << "x" << g.ny << " cap=" << g.capacity << "\n";
        std::cerr << "  h_use:";
        for (int u : g.h_use) {
            std::cerr << " " << u;
        }
        std::cerr << "\n  v_use:";
        for (int u : g.v_use) {
            std::cerr << " " << u;
        }
        std::cerr << "\n";
        return 1;
    }
    std::cout << "PASS " << label << " wl=" << got << "\n";
    return 0;
}

int main() {
    int fails = 0;

    // 2-pin: (0,0)-(15,0) on 20x10, tile 10 → 2x1, one H hop.
    {
        minipd::Design d = make_grid(20, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 15, 0));
        add_net(d, {0, 1});
        fails += route_wl(d, "2pin-h", 1);
    }

    // 2-pin L: (0,0)-(20,20) on 30x30 → (0,0) to (2,2), 4 hops.
    {
        minipd::Design d = make_grid(30, 30, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 20, 20));
        add_net(d, {0, 1});
        fails += route_wl(d, "2pin-L", 4);
    }

    // 3-pin line: gcells 0,1,2. MST is two hops, not the long edge.
    {
        minipd::Design d = make_grid(30, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 10, 0));
        d.cells.push_back(make_cell("c", 20, 0));
        add_net(d, {0, 1, 2});
        fails += route_wl(d, "3pin-line", 2);
    }

    // 3-pin L: (0,0),(2,0),(0,2). MST = 2+2, not the diagonal 4.
    {
        minipd::Design d = make_grid(30, 30, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 20, 0));
        d.cells.push_back(make_cell("c", 0, 20));
        add_net(d, {0, 1, 2});
        fails += route_wl(d, "3pin-L", 4);
    }

    // Two pins in the same G-cell, one hop to the third.
    {
        minipd::Design d = make_grid(20, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 1, 0));
        d.cells.push_back(make_cell("c", 15, 0));
        add_net(d, {0, 1, 2});
        fails += route_wl(d, "3pin-same-gcell", 1);
    }

    // All pins in one G-cell: no wires.
    {
        minipd::Design d = make_grid(20, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 1, 0));
        d.cells.push_back(make_cell("c", 2, 0));
        add_net(d, {0, 1, 2});
        fails += route_wl(d, "3pin-all-same", 0);
    }

    // 4-pin two clusters: ix 0,1,3,4. Kruskal: 0-1, 3-4, bridge 1-3. wl=4.
    // Union that does not find() the root also takes 1-4 and gets wl=7.
    {
        minipd::Design d = make_grid(50, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 10, 0));
        d.cells.push_back(make_cell("c", 30, 0));
        d.cells.push_back(make_cell("d", 40, 0));
        add_net(d, {0, 1, 2, 3});
        fails += route_wl(d, "4pin-two-clusters", 4);
    }

    // 4-pin square: MST is 3 sides = 6, not both diagonals (8).
    {
        minipd::Design d = make_grid(30, 30, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        d.cells.push_back(make_cell("b", 20, 0));
        d.cells.push_back(make_cell("c", 0, 20));
        d.cells.push_back(make_cell("d", 20, 20));
        add_net(d, {0, 1, 2, 3});
        fails += route_wl(d, "4pin-square", 6);
    }

    // 1-pin net is skipped.
    {
        minipd::Design d = make_grid(20, 10, 10);
        d.cells.push_back(make_cell("a", 0, 0));
        add_net(d, {0});
        fails += route_wl(d, "1pin-skip", 0);
    }

    if (fails) {
        std::cerr << fails << " case(s) failed\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
