#pragma once

#include "place/placer.hpp"

#include <cstdint>

namespace minipd {

class RandomPlacer final : public IPlacer {
public:
    explicit RandomPlacer(std::uint32_t seed = 1);
    void place(Design& design) override;
    const char* name() const override;

private:
    std::uint32_t seed_;
};

}  // namespace minipd
