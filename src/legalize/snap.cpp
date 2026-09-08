#include "legalize/snap.hpp"

namespace minipd {

const char* SnapLegalizer::name() const {
    return "snap (no-op)"; //Placeholder name so placer can still run but remind to finish this
}

void SnapLegalizer::legalize(Design& design) {
    
    //Will finish writing this and commit later
    //Goal is to snap to nearest row, then populate left to right to attempt a basic legalization
    //Will certainly be updated with a better version later

    (void)design;
}

}  // namespace minipd
