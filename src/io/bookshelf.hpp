#pragma once

#include "ir/design.hpp"

#include <string>

namespace minipd {

// Bookshelf (UCLA / ISPD row-based). Entry file is .aux; siblings sit next to it.
// v1 subset: .nodes .nets .scl. Ignore .wts and .pl. No terminals, no pin offsets.
//
// .aux
//   RowBasedPlacement : <file> <file> ...
//
// .nodes
//   UCLA nodes 1.0
//   NumNodes : <n>
//   NumTerminals : 0
//   <name> <width> <height>
//
// .nets
//   UCLA nets 1.0
//   NumNets : <n>
//   NumPins : <p>
//   NetDegree : <degree> <netname>
//     <cell> <I|O|B> : <dx> <dy>     // v1: ignore I|O|B and dx,dy; pin = cell LL
//
// .scl
//   UCLA scl 1.0
//   NumRows : <n>
//   CoreRow Horizontal
//     Coordinate    : <y>            // row bottom, same as Row::y
//     Height        : <h>
//     Sitewidth     : <sw>
//     Sitespacing   : <ss>
//     Siteorient    : N
//     Sitesymmetry  : Y
//     SubrowOrigin  : <x0>  NumSites : <ns>
//   End
//   Row::x0 = SubrowOrigin
//   Row::x1 = SubrowOrigin + NumSites * Sitewidth
//
// Die bbox is the union of rows (Bookshelf has no DIE record).
// # starts a comment. Header lines "UCLA ... 1.0" are skipped.
Design parse_bookshelf(const std::string& aux_path);

}  // namespace minipd
