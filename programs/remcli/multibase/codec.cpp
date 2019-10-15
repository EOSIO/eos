#include "codec.h"
#include <multibase/codec.h>
#include <multibase/detail/codec_impl.h>

namespace multibase {

codec::codec(encoding base) : pImpl(codec::impl::registry()[base].get()) {
  if (!pImpl) {
    auto msg = std::string("No codec implementation for encoding ");
    msg.push_back(static_cast<char>(base));
    throw std::out_of_range(msg);
  }
}

std::string codec::encode(std::string_view input, bool include_encoding) {
  return pImpl->encode(input, include_encoding);
}

std::size_t codec::encode(std::string_view input, std::string& output,
                          bool include_encoding) {
  return pImpl->encode(input, output, include_encoding);
}

std::size_t codec::encoded_size(std::string_view input,
                                bool include_encoding) {
  return pImpl->encoded_size(input, include_encoding);
}

std::string codec::decode(std::string_view input) {
  return pImpl->decode(input);
}

std::size_t codec::decode(std::string_view input, std::string& output) {
  return pImpl->decode(input, output);
}

std::size_t codec::decoded_size(std::string_view input) {
  return pImpl->decoded_size(input);
}

encoding codec::base() const { return pImpl->base(); }

bool codec::is_valid(std::string_view input, bool include_encoding) {
  return pImpl->is_valid(input, include_encoding);
}

}  // namespace multibase
