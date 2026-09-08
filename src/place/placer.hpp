#pragma once

#include "ir/design.hpp"

namespace minipd {

// Global placement. Mutates Cell::x/y and Cell::placed.
//Start with true random, then move to quadratic/force
class IPlacer {
public:
    virtual ~IPlacer() = default;
    virtual void place(Design& design) = 0;
    virtual const char* name() const = 0;
};

}  // namespace minipd
