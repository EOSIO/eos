#pragma once

#include <string>
#include <vector>

namespace eos { namespace utilities {
std::vector<uint8_t> assemble_wast(const std::string& wast);
} } // namespace eos::utilities
