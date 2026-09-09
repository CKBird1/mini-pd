#pragma once

#include "place/placer.hpp"
#include <vector>
#include <cstdint>

namespace minipd {

class QuadraticPlacer final : public IPlacer {
public:
    explicit QuadraticPlacer(std::uint32_t seed = 1);
    void place(Design& design) override;
    const char* name() const override;

private:
    std::vector<double> forwardElimBackSub(const std::vector<std::vector<double>>& L, const std::vector<double>& B);
    std::uint32_t seed_;
};

}  // namespace minipd
