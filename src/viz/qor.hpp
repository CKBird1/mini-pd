#pragma once

#include "ir/design.hpp"

#include <string>

namespace minipd {

void write_qor(const Design& design,
               const std::string& input_path,
               const std::string& placer_name,
               const std::string& legalizer_name,
               double hpwl_value,
               const std::string& path);

}  // namespace minipd
