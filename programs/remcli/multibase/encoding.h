#pragma once

namespace multibase {

using encoding_t = unsigned char;

enum class encoding : encoding_t {
  base_unknown = 0,
  base_16 = 'f',
  base_16_upper = 'F',
  base_32 = 'b',
  base_32_upper = 'B',
  base_58_btc = 'Z',
  base_64 = 'm'
};

}  // namespace multibase
