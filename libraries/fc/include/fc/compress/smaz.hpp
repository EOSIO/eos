#pragma once
#include <string>

namespace fc {

  std::string smaz_compress( const std::string& in );
  std::string smaz_decompress( const std::string& compressed );

} // namespace fc
