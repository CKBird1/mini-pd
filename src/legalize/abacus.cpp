#include "legalize/abacus.hpp"

namespace minipd {

const char* AbacusLegalizer::name() const {
    return "abacus";
}

void AbacusLegalizer::legalize(Design& /*design*/) {
}

// Trial-insert `cell` into `row`. Return a cost for this trial (you define it).
double AbacusLegalizer::placeCell(Design& /*design*/, std::size_t /*cell*/, std::size_t /*row*/) {
    return 0.0;
}

// Pack the cells already assigned to `row` (clusters / optimal x).
void AbacusLegalizer::placeRow(Design& /*design*/, std::size_t /*row*/) {
}

}  // namespace minipd
