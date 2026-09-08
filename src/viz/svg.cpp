#include "viz/svg.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace minipd {
namespace {

std::string xml_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':
                o += "&amp;";
                break;
            case '<':
                o += "&lt;";
                break;
            case '>':
                o += "&gt;";
                break;
            case '"':
                o += "&quot;";
                break;
            default:
                o += c;
                break;
        }
    }
    return o;
}

}  // namespace

void write_placed_svg(const Design& design, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot write " + path);
    }

    const Rect& die = design.die.bbox;
    const double dw = die.width();
    const double dh = die.height();
    if (dw <= 0 || dh <= 0) {
        throw std::runtime_error("cannot draw empty die");
    }

    const double px = 800.0;
    const double py = px * dh / dw;

    // viewBox y=die.y0 is the top of the picture; layout y=die.y0 is the bottom.
    auto svg_y = [&](double layout_y) { return die.y0 + die.y1 - layout_y; };

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "viewBox=\"" << die.x0 << " " << die.y0 << " " << dw << " " << dh << "\" "
        << "width=\"" << px << "\" height=\"" << py << "\">\n";
    out << "  <rect x=\"" << die.x0 << "\" y=\"" << die.y0 << "\" width=\"" << dw
        << "\" height=\"" << dh << "\" fill=\"#f7f7f7\" stroke=\"#222\" "
        << "stroke-width=\"" << (dw * 0.003) << "\"/>\n";

    for (const Row& row : design.rows) {
        const double y = svg_y(row.y);
        out << "  <line x1=\"" << row.x0 << "\" y1=\"" << y << "\" x2=\"" << row.x1
            << "\" y2=\"" << y << "\" stroke=\"#ccc\" stroke-width=\""
            << (dw * 0.0015) << "\"/>\n";
    }

    const double net_sw = dw * 0.002;
    for (const Net& net : design.nets) {
        if (net.pins.size() < 2) {
            continue;
        }
        out << "  <polyline fill=\"none\" stroke=\"#c44\" stroke-opacity=\"0.35\" "
            << "stroke-width=\"" << net_sw << "\" points=\"";
        for (std::size_t i = 0; i < net.pins.size(); ++i) {
            const Cell& c = design.cells[net.pins[i]];
            if (i) {
                out << " ";
            }
            out << c.x << "," << svg_y(c.y);
        }
        out << "\"/>\n";
    }

    const double font = dw * 0.018;
    for (const Cell& cell : design.cells) {
        const double y = svg_y(cell.y + cell.height);
        out << "  <rect x=\"" << cell.x << "\" y=\"" << y << "\" width=\""
            << cell.width << "\" height=\"" << cell.height
            << "\" fill=\"#8cb4e8\" stroke=\"#1f4e79\" stroke-width=\""
            << (dw * 0.0015) << "\"/>\n";
        out << "  <text x=\"" << (cell.x + cell.width * 0.08) << "\" y=\""
            << (y + cell.height * 0.72) << "\" font-size=\"" << font
            << "\" font-family=\"sans-serif\" fill=\"#123\">"
            << xml_escape(cell.name) << "</text>\n";
    }

    out << "</svg>\n";
}

}  // namespace minipd
