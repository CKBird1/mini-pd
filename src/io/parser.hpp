#pragma once

#include "ir/design.hpp"

#include <string>

namespace minipd {

// Tiny format (one record per line, # comments allowed):
//   DIE  <x0> <y0> <x1> <y1>
//   ROWS <count> <height>     // rows stack up from die.y0; leftover die height is OK
//   CELL <name> <width> <height>
//   NET  <name> <cell> <cell> ...
Design parse_file(const std::string& path);

}  // namespace minipd
