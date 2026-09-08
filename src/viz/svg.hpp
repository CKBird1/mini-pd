#pragma once

#include "ir/design.hpp"

#include <string>

namespace minipd {

// Writes out/placed.svg. Layout y-up is flipped to SVG y-down so die (x0,y0)
// appears at the bottom-left of the picture. Net lines are a debug overlay,
// not a route.
void write_placed_svg(const Design& design, const std::string& path);

}  // namespace minipd
