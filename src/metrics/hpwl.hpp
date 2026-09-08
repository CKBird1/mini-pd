#pragma once

#include "ir/design.hpp"

namespace minipd {

// Half-perimeter wirelength as basic QoR check for placed design
// Can/will implement better, more useful metrics later

double hpwl(const Design& design);

}  // namespace minipd
