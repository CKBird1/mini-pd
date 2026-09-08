#include "ir/design.hpp"
#include "metrics/hpwl.hpp"

#include <cmath>
#include <iostream>

// Pins are all assumed at lower left corner of cell, will change later

static minipd::Cell make_cell(const char* name, double w, double h, double x, double y) {
    minipd::Cell c;
    c.name = name;
    c.width = w;
    c.height = h;
    c.x = x;
    c.y = y;
    c.placed = true;
    return c;
}

//Flow agnostic test to ensure implemented algs work the way they should. Good simple testing practice

int main() {
    minipd::Design d;
    d.die.bbox = {0, 0, 100, 100};

    // Three pins: (0,0), (10,5), (3,20). BBox is 10 x 20. HPWL = 30.
    // MST of those points is 37, so a spanning-tree sum is the wrong metric.
    d.cells.push_back(make_cell("a", 4, 2, 0, 0));
    d.cells.push_back(make_cell("b", 8, 4, 10, 5));
    d.cells.push_back(make_cell("c", 4, 2, 3, 20));

    minipd::Net n3;
    n3.name = "n3";
    n3.pins = {0, 1, 2};
    d.nets.push_back(n3);

    minipd::Net n1;
    n1.name = "n1";
    n1.pins = {0};
    d.nets.push_back(n1);

    const double got = minipd::hpwl(d);
    const double want = 30.0;
    if (std::abs(got - want) > 1e-9) {
        std::cerr << "HPWL Mismatch: hpwl=" << got << " want=" << want << "\n";
        return 1;
    }

    std::cout << "PASS\n";
    return 0;
}
