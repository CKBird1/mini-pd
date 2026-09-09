#include "place/quadratic.hpp"
#include "place/random.hpp"
#include <cmath>
#include <algorithm>
#include <cassert>

constexpr double wa = 0.01;

namespace minipd {

QuadraticPlacer::QuadraticPlacer(std::uint32_t seed) : seed_(seed) {}

const char* QuadraticPlacer::name() const {
    return "quadratic";
}

std::vector<double> QuadraticPlacer::forwardElimBackSub(const std::vector<std::vector<double>>& L, const std::vector<double>& B) {
    //Need copy of L and b so I can mess with it without ruining originals
    std::vector<std::vector<double>> Lcopy(L);
    std::vector<double> Bcopy(B);

    
    std::size_t n = Bcopy.size();
    if(n == 0) return Bcopy;
    std::vector<double> x(n); //Final results
    
    double fabsMax = 0.0;
    size_t maxRow = 0;
    for(std::size_t k = 0; k < n-1; ++k) {
        maxRow = k;
        fabsMax = std::fabs(Lcopy[k][k]);
        for(std::size_t row = k+1; row < n; ++row) {
            if(std::fabs(Lcopy[row][k]) > fabsMax) {
                fabsMax = std::fabs(Lcopy[row][k]);
                maxRow = row;
            }
        }
        if(maxRow != k) {
            std::swap(Lcopy[k], Lcopy[maxRow]);
            std::swap(Bcopy[k], Bcopy[maxRow]);
        }
        assert(std::fabs(Lcopy[k][k]) > 1e-15);

        //Now actual elimination
        for(std::size_t i = k+1; i < n; ++i) {
            double f = Lcopy[i][k] / Lcopy[k][k];
            for(std::size_t j = k; j < n; ++j) {
                Lcopy[i][j] -= f * Lcopy[k][j];
            }
            Bcopy[i] -= f * Bcopy[k];
        }
    }
    
    assert(std::fabs(Lcopy[n-1][n-1]) > 1e-15);
    x[n-1] = Bcopy[n-1]/Lcopy[n-1][n-1];
    for(int i = (int)n-2; i >= 0; --i) {
        double sum = 0.0;
        for(std::size_t j = i+1; j < n; ++j) {
            sum += Lcopy[i][j] * x[j];
        }
        x[i] = (Bcopy[i] - sum) / Lcopy[i][i];
    }            


    return x;
}

void QuadraticPlacer::place(Design& design) {
    //Initial placement, simply random for now
    RandomPlacer init(seed_);
    init.place(design);

    //Now create data structures based on current objects. we work on these values and write at once at the end
    std::vector<double> cells_x(design.cells.size());
    std::vector<double> cells_y(design.cells.size());
    std::vector<char> to_solve(design.cells.size());

    for(size_t i = 0; i < design.cells.size(); ++i) {
        cells_x[i] = design.cells[i].x;
        cells_y[i] = design.cells[i].y;
        if((design.cells[i].width > design.die.bbox.width()) || (design.cells[i].height > design.die.bbox.height()))
            to_solve[i] = 0;
        else to_solve[i] = 1;
    }

    //Now have all relevant data, create Laplacian matrix of springs and begin work
    //using vector vector double because current datasets are SMALL, once they get to any realistic
    //size switch to using a much more efficient data structure.

    std::size_t n = design.cells.size();
    std::vector<std::vector<double>> L(n, std::vector<double>(n, 0.0));

    for(std::size_t i = 0; i < design.nets.size(); ++i) {
        const auto& pins = design.nets[i].pins;
        if(pins.size() < 2) continue;
        double weight = (1.0 / (pins.size() - 1));
        for(std::size_t j = 0; j < pins.size(); ++j) {
            for(std::size_t k = j+1; k < pins.size(); ++k) {
                const std::size_t a = pins[j];
                const std::size_t b = pins[k];
                L[a][a] += weight;
                L[b][b] += weight;
                L[a][b] -= weight;
                L[b][a] -= weight;
            }
        }
    }

    std::vector<double> b_x(n, 0.0);
    std::vector<double> b_y(n, 0.0);

    for(std::size_t c = 0; c < n; ++c) {
        if(to_solve[c] == 0) continue;
        L[c][c] += wa;
        b_x[c] += (wa * cells_x[c]); //Initial placement is dummy anchor
        b_y[c] += (wa * cells_y[c]);
    }

    // Oversized cells stay at init and act as fixed pins (Dirichlet rows).
    for(std::size_t c = 0; c < n; ++c) {
        if(to_solve[c] == 1) continue;
        std::fill(L[c].begin(), L[c].end(), 0.0);
        L[c][c] = 1.0;
        b_x[c] = cells_x[c];
        b_y[c] = cells_y[c];
    }

    //Now solve the Laplace matrix with forward elim and back sub, and finally rewrite to design.cells
    std::vector<double> x;
    std::vector<double> y;
    x = forwardElimBackSub(L, b_x);
    y = forwardElimBackSub(L, b_y);

    const double lowX = design.die.bbox.x0;
    const double lowY = design.die.bbox.y0;
    for(std::size_t d = 0; d < n; ++d) {
        if(to_solve[d] == 0) continue;
        const double highX = design.die.bbox.x1 - design.cells[d].width;
        const double highY = design.die.bbox.y1 - design.cells[d].height;


        design.cells[d].x = std::min(highX, std::max(lowX, x[d]));
        design.cells[d].y = std::min(highY, std::max(lowY, y[d]));
        design.cells[d].placed = true; 
        design.cells[d].legal = false;
    }

}

}  // namespace minipd
