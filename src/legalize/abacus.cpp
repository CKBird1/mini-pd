#include "legalize/abacus.hpp"
#include <algorithm>
#include <cmath>

namespace minipd {

const char* AbacusLegalizer::name() const {
    return "abacus";
}

void AbacusLegalizer::legalize(Design& design) {
    std::size_t n = design.cells.size();
    orig_x_.resize(n);
    orig_y_.resize(n);
    row_cells_.assign(design.rows.size(), {});
    for(std::size_t i = 0; i < n; ++i) {
        orig_x_[i] = design.cells[i].x;
        orig_y_[i] = design.cells[i].y;
    }

    //Now sort order by x value, because abacus is sorted by x normally
    std::vector<std::size_t> travOrder(n);
    for(std::size_t j = 0; j < n; ++j) travOrder[j] = j;
    std::sort(travOrder.begin(), travOrder.end(), [&](std::size_t a, std::size_t b) {
        return orig_x_[a] < orig_x_[b]; });

    if(design.rows.empty()) return;
    for(std::size_t cell : travOrder) {
        std::size_t best_row = -1;
        double best_cost = INFINITY;
        for(std::size_t k = 0; k < design.rows.size(); ++k) {
            double cost = placeCell(design, cell, k);
            if(cost < best_cost) {
                best_row = k;
                best_cost = cost;
            }
            row_cells_[k].pop_back();
        }
        placeCell(design, cell, best_row); //Full commit
        bool legal = (design.cells[cell].x >= design.rows[best_row].x0) && ((design.cells[cell].x + design.cells[cell].width) <= design.rows[best_row].x1);
        design.cells[cell].legal = legal;
    }
    

}

// Trial-insert `cell` into `row`. Return a cost for this trial (you define it).
double AbacusLegalizer::placeCell(Design& design, std::size_t cell, std::size_t row) {
    row_cells_[row].push_back(cell);
    design.cells[cell].y = design.rows[row].y;
    if(row_cells_[row].size() == 1) design.cells[cell].x = std::max(design.rows[row].x0, orig_x_[cell]);
    else {
        std::size_t prev = row_cells_[row][row_cells_[row].size() -2];
        design.cells[cell].x = std::max({design.rows[row].x0, orig_x_[cell], design.cells[prev].x + design.cells[prev].width});
    }
    //placeRow(design, row); Not filled yet, will fill for true abacus

    double deltax = design.cells[cell].x - orig_x_[cell];
    double deltay = design.cells[cell].y - orig_y_[cell];
    return (deltax*deltax) + (deltay*deltay); 
}

// Pack the cells already assigned to `row` (clusters / optimal x).
void AbacusLegalizer::placeRow(Design& /*design*/, std::size_t /*row*/) {
    //Currently not used, cells don't slide, clusters next
}
    
}  // namespace minipd
