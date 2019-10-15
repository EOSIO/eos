#include "codec_impl.h"
#include <multibase/basic_codec.h>
#include <multibase/detail/codec_impl.h>

namespace multibase {

std::size_t codec::impl::encoding_size(bool include_encoding) {
  return include_encoding ? sizeof(encoding_t) : 0;
}

std::size_t codec::impl::encoded_size(std::string_view input,
                                      bool include_encoding) {
  return get_encoded_size(input) + encoding_size(include_encoding);
}

std::size_t codec::impl::write_encoding(std::string& output,
                                        bool include_encoding) {
  if (!include_encoding) return 0;
  auto bytes_written = encoding_size(include_encoding);
  if (bytes_written > output.size())
    throw std::out_of_range("Output buffer too small");
  auto data = get_encoding();
  std::copy(reinterpret_cast<char*>(&data),
            reinterpret_cast<char*>(&data + sizeof(data)),
            output.data());
  return bytes_written;
}

std::string codec::impl::encode(std::string_view input,
                                bool include_encoding) {
  auto result = std::string(encoded_size(input, include_encoding), 0);
  auto size = encode(input, result, include_encoding);
  size += include_encoding ? 1 : 0;
  result.resize(size);
  return result;
}

std::size_t codec::impl::encode(std::string_view input, std::string& output,
                                bool include_encoding) {
  auto basic_size = get_encoded_size(input);
  auto size = basic_size + encoding_size(include_encoding);
  if (output.size() < size) throw std::out_of_range("Output buffer too small");
  std::string tmp(basic_size, 0);
  const auto ret = encode(input, tmp, impl_tag{});
  std::copy(std::begin(tmp), std::end(tmp), std::begin(output) + write_encoding(output, include_encoding));
  return ret;
}

std::size_t codec::impl::decoded_size(std::string_view input) {
  return get_decoded_size(input);
}

std::size_t codec::impl::decode(std::string_view input,
                                std::string& output) {
  if (get_decoded_size(input) > output.size())
    throw std::out_of_range("Output buffer too small");
  return decode(input, output, impl_tag{});
}

std::string codec::impl::decode(std::string_view input) {
  auto size = decoded_size(input);
  auto output = std::string(size, 0);
  output.resize(decode(input, output));
  return output;
}

encoding codec::impl::base() { return get_encoding(); }

bool codec::impl::is_valid(std::string_view input, bool include_encoding) {
  auto valid = true;
  if (include_encoding) {
    if (input.empty()) {
      valid = false;
    } else {
      valid = input[0] == static_cast<char>(base());
    }
  }
  if (valid) {
    valid = is_valid(input, impl_tag{});
  }
  return valid;
}

codec::impl::registry::data_type& codec::impl::registry::data() {
  static data_type data_ =
      data_type{{encoding::base_16, std::make_unique<base_16>()},
                {encoding::base_58_btc, std::make_unique<base_58_btc>()}};
  return data_;
}

codec::impl::registry::mapped_type& codec::impl::registry::operator[](
    const codec::impl::registry::key_type& key) {
  return data()[key];
}

}  // namespace multibase
