#include "place/random.hpp"

#include <random>

namespace minipd {

RandomPlacer::RandomPlacer(std::uint32_t seed) : seed_(seed) {}

const char* RandomPlacer::name() const {
    return "random";
}

void RandomPlacer::place(Design& design) {
    std::mt19937 rng(seed_);
    
    for (auto& cell : design.cells) {
        if((cell.width > design.die.bbox.width()) || (cell.height > design.die.bbox.height())) {
            cell.x = design.die.bbox.x0;
            cell.y = design.die.bbox.y0;
            cell.placed = true;
            cell.legal = false;
            continue; 
        } //if cell is bigger than die, clamp to bottom left and skip generation, this will be default going forward

        double lowX = design.die.bbox.x0;
        double lowY = design.die.bbox.y0; 
        double highX = design.die.bbox.x1 - cell.width;
        double highY = design.die.bbox.y1 - cell.height;

        std::uniform_real_distribution<double> dist_x(lowX, highX);
        std::uniform_real_distribution<double> dist_y(lowY, highY);
        cell.x = dist_x(rng);
        cell.y = dist_y(rng);
        cell.placed = true;
        cell.legal = false;
    }
}

}  // namespace minipd
