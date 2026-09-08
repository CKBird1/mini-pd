#pragma once

#include "ir/design.hpp"

namespace minipd {

// Detailed legalization. Mutates Cell::x/y and Cell::legal.
//Snap first, then abacus
class ILegalizer {
public:
    virtual ~ILegalizer() = default;
    virtual void legalize(Design& design) = 0;
    virtual const char* name() const = 0;
};

}  // namespace minipd
