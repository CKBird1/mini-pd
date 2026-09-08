#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace minipd {

// Axis-aligned box. Layout coordinates: x right, y up, origin at die lower-left.
struct Rect {
    double x0 = 0;
    double y0 = 0;
    double x1 = 0;
    double y1 = 0;

    double width() const { return x1 - x0; }
    double height() const { return y1 - y0; }
};

struct Die {
    Rect bbox;
};

// Horizontal standard-cell row. y is the bottom edge.
struct Row {
    double y = 0;
    double height = 0;
    double x0 = 0;
    double x1 = 0;
};

// Movable instance. (x, y) is the lower-left of the cell bbox.
// Currently no orientation, pin at lower left, always one pin
struct Cell {
    std::string name;
    double width = 0;
    double height = 0;
    double x = 0;
    double y = 0;
    bool placed = false;
    bool legal = false;
};

// Hyperedge. pins[i] is an index into Design::cells.
struct Net {
    std::string name;
    std::vector<std::size_t> pins;
};

struct Design {
    Die die;
    std::vector<Row> rows;
    std::vector<Cell> cells;
    std::vector<Net> nets;
};

}  // namespace minipd
