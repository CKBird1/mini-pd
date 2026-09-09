#pragma once

#include "legalize/legalizer.hpp"

#include <cstddef>

namespace minipd {

class AbacusLegalizer final : public ILegalizer {
public:
    void legalize(Design& design) override;
    const char* name() const override;

private:
    // Paper: PlaceCell / PlaceRow. Change the signatures as you implement.
    double placeCell(Design& design, std::size_t cell, std::size_t row);
    void placeRow(Design& design, std::size_t row);
};

}  // namespace minipd
