#include "legalize/snap.hpp"
#include <cmath> 
#include <algorithm>

namespace minipd {

const char* SnapLegalizer::name() const {
    return "snap"; 
}

void SnapLegalizer::legalize(Design& design) {
    //Will certainly be updated with a better version later

    std::vector<std::vector<std::size_t>> sortedByRows(design.rows.size());

    for(std::size_t i = 0; i < design.cells.size(); ++i) {
        int j = static_cast<int>(std::round((design.cells[i].y - design.rows[0].y) / design.rows[0].height));
        j = std::max(0, std::min(j, static_cast<int>(design.rows.size()-1))); //Guarantee we snap to 0 through num rows -1
        design.cells[i].y = design.rows[j].y;

        //We're happy with i, can use that info to pack cells into rows for local iteration and then dump the sort later
        sortedByRows[j].push_back(i);
    }

    for(std::size_t k = 0; k < sortedByRows.size(); ++k) {
        std::sort(sortedByRows[k].begin(), sortedByRows[k].end(), [&](std::size_t a, std::size_t b) {
            return design.cells[a].x < design.cells[b].x;
        });    

        double currPlacement = design.rows[k].x0;
        for(std::size_t l = 0; l < sortedByRows[k].size(); ++l) {
            Cell& c =  design.cells[sortedByRows[k][l]];
            c.x = currPlacement;
            currPlacement += c.width; 

            if(currPlacement <= design.rows[k].x1) c.legal = true;
        }
        
    }


}

}  // namespace minipd
