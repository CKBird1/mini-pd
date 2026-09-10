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
            std::vector<double> prevX(row_cells_[k].size());
            for(std::size_t l = 0; l < row_cells_[k].size(); ++l) prevX[l] = design.cells[row_cells_[k][l]].x;
            double cost = placeCell(design, cell, k);
            if(cost < best_cost) {
                best_row = k;
                best_cost = cost;
            }
            row_cells_[k].pop_back();
            for(std::size_t l = 0; l < row_cells_[k].size(); ++l) design.cells[row_cells_[k][l]].x = prevX[l];
        }
        placeCell(design, cell, best_row); //Full commit
        bool legal = (design.cells[cell].x >= design.rows[best_row].x0) && ((design.cells[cell].x + design.cells[cell].width) <= design.rows[best_row].x1);
        design.cells[cell].legal = legal;
    }


}

// Trial-insert cell into row.
double AbacusLegalizer::placeCell(Design& design, std::size_t cell, std::size_t row) {
    row_cells_[row].push_back(cell);
    design.cells[cell].y = design.rows[row].y;
    placeRow(design, row);

    double cost = 0.0;
    for(std::size_t i = 0; i < row_cells_[row].size(); ++i) {
        std::size_t nc = row_cells_[row][i];
        double deltax = design.cells[nc].x - orig_x_[nc];
        double deltay = design.cells[nc].y - orig_y_[nc];
        cost += (deltax*deltax)+(deltay*deltay);
    }

    return cost;
}

struct Cluster {
    std::size_t rowStart;
    std::size_t rowEnd;
    double leftEdge;
    double weight;
    double accumulator;
    double widthOfAll;

    Cluster(std::size_t s, std::size_t e, double l, double w, double q, double woa) :
        rowStart(s), rowEnd(e), leftEdge(l), weight(w), accumulator(q), widthOfAll(woa) {}
};

void AbacusLegalizer::placeRow(Design& design, std::size_t row) {
    std::vector<Cluster> clusters;
    for(std::size_t k = 0; k < row_cells_[row].size(); ++k) {
        std::size_t cell = row_cells_[row][k]; //Make a cluster for each cell and pack into clusters
        double le = std::max(design.rows[row].x0, std::min((orig_x_[cell]), design.rows[row].x1 - design.cells[cell].width));
        clusters.push_back(Cluster(k, k, le, 1, orig_x_[cell], design.cells[cell].width));

        //now try to collapse the clusters if they overlap
        while(clusters.size() >= 2 && (clusters.back().leftEdge < (clusters[clusters.size()-2].leftEdge + clusters[clusters.size()-2].widthOfAll))) {
            Cluster& mergeInto = clusters[clusters.size()-2];
            mergeInto.rowEnd = clusters.back().rowEnd;
            mergeInto.weight += clusters.back().weight;
            mergeInto.accumulator += clusters.back().accumulator - clusters.back().weight * mergeInto.widthOfAll; //Before updating widthOfAll
            mergeInto.widthOfAll += clusters.back().widthOfAll;
            mergeInto.leftEdge = std::max(design.rows[row].x0, std::min((mergeInto.accumulator/mergeInto.weight), design.rows[row].x1 - mergeInto.widthOfAll));
            clusters.pop_back();
        }
    }

    for(const auto& clus : clusters) {
        double x = clus.leftEdge;
        for(std::size_t k = clus.rowStart; k <= clus.rowEnd; ++k) {
            design.cells[row_cells_[row][k]].x = x;
            x += design.cells[row_cells_[row][k]].width;
        }
    }
}

}  // namespace minipd
