#pragma once

#include "legalize/legalizer.hpp"

#include <cstddef>

namespace minipd {

class AbacusLegalizer final : public ILegalizer {
public:
    void legalize(Design& design) override;
    const char* name() const override;

private:
    double placeCell(Design& design, std::size_t cell, std::size_t row);
    void placeRow(Design& design, std::size_t row);
    
    std::vector<double> orig_x_;
    std::vector<double> orig_y_;
    std::vector<std::vector<std::size_t>> row_cells_; //Cell indicies per row
};

}  // namespace minipd
