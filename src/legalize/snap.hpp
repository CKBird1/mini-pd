#pragma once

#include "legalize/legalizer.hpp"

namespace minipd {

class SnapLegalizer final : public ILegalizer {
public:
    void legalize(Design& design) override;
    const char* name() const override;
};

}  // namespace minipd
