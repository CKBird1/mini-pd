#include "viz/qor.hpp"

#include <fstream>
#include <stdexcept>

namespace minipd {

void write_qor(const Design& design,
               const std::string& input_path,
               const std::string& placer_name,
               const std::string& legalizer_name,
               double hpwl_value,
               const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot write " + path);
    }

    std::size_t n_placed = 0;
    std::size_t n_legal = 0;
    for (const Cell& c : design.cells) {
        if (c.placed) {
            ++n_placed;
        }
        if (c.legal) {
            ++n_legal;
        }
    }

    const Rect& die = design.die.bbox;
    const double row_h = design.rows.empty() ? 0.0 : design.rows.front().height;

    out << "input: " << input_path << "\n";
    out << "placer: " << placer_name << "\n";
    out << "legalizer: " << legalizer_name << "\n";
    out << "die: " << die.x0 << " " << die.y0 << " " << die.x1 << " " << die.y1 << "\n";
    out << "rows: " << design.rows.size() << " height=" << row_h << "\n";
    out << "cells: " << design.cells.size() << "\n";
    out << "nets: " << design.nets.size() << "\n";
    out << "placed: " << n_placed << "\n";
    out << "legal: " << n_legal << "\n";
    out << "hpwl: " << hpwl_value << "\n";
}

}  // namespace minipd
