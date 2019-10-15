#include "multibase.h"
#include <multibase/multibase.h>

namespace multibase {

std::string encode(std::string_view input, encoding base,
                   bool include_encoding) {
  return codec(base).encode(input, include_encoding);
}

std::size_t encode(std::string_view input, std::string& output,
                   encoding base, bool include_encoding) {
  return codec(base).encode(input, output, include_encoding);
}

std::size_t encoded_size(std::string_view input, encoding base,
                         bool include_encoding) {
  return codec(base).encoded_size(input, include_encoding);
}

std::string decode(std::string_view input, encoding base) {
  if (input.empty()) return std::string();
  auto mb = multibase(input, base);
  auto decoder = codec(mb.base());
  auto view = std::string_view(&input[0] + 1, input.size() - 1);
  return decoder.decode(view);
}

std::size_t decode(std::string_view input, std::string& output,
                   encoding base) {
  auto mb = multibase(input, base);
  return codec(mb.base()).decode(mb.encoded_data(), output);
}

std::size_t decoded_size(std::string_view input, encoding base) {
  auto mb = multibase(input, base);
  return codec(mb.base()).decoded_size(input);
}

bool is_valid(std::string_view input, encoding base) {
  auto result = true;
  try {
    auto mb = multibase(input, base);
    result = is_valid(mb);
  } catch (std::range_error& e) {
    result = false;
  }
  return result;
}

bool is_valid(const multibase& value) {
  return codec(value.base()).is_valid(value.encoded_data(), false);
}

multibase::multibase(std::string_view data, encoding base)
    : data_(data), base_(base) {
  if (base_ == encoding::base_unknown) {
    if (data_.length() < 2) {
      throw std::range_error("multibase must be at least 2 characters");
    }
    base_ = static_cast<encoding>(data_[0]);
    data_ = std::string_view(&data_[1], data_.size() - 1);
  }
}

encoding multibase::base() const { return base_; }

std::string_view multibase::encoded_data() const { return data_; }

bool multibase::operator==(const multibase& rhs) const {
  return data_ == rhs.data_;
}

bool multibase::operator!=(const multibase& rhs) const {
  return !(*this == rhs);
}

bool multibase::operator<(const multibase& rhs) const {
  return data_ < rhs.data_;
}

bool multibase::operator>(const multibase& rhs) const {
  return data_ > rhs.data_;
}
}  // namespace multibase
