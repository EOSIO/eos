#pragma once

#include <multibase/basic_codec.h>

namespace multibase {

class multibase {
 public:
  multibase() = default;
  explicit multibase(std::string_view data, encoding base = encoding::base_unknown);

  encoding base() const;
  std::string_view encoded_data() const;

  bool operator==(const multibase& rhs) const;
  bool operator!=(const multibase& rhs) const;
  bool operator<(const multibase& rhs) const;
  bool operator>(const multibase& rhs) const;

 private:
  std::string_view data_ = std::string_view();
  encoding base_ = encoding::base_unknown;
};

/** Determine whether the input is a valid encoding in the embedded base */
bool is_valid(const multibase& input);

bool is_valid(std::string_view input,
              encoding base = encoding::base_unknown);

std::string encode(std::string_view input, encoding base,
                   bool include_encoding = true);

std::size_t encode(std::string_view input, std::string& output,
                   encoding base, bool include_encoding = true);

std::size_t encoded_size(std::string_view input, encoding base,
                         bool include_encoding = true);

std::string decode(std::string_view input,
                   encoding base = encoding::base_unknown);

std::size_t decode(std::string_view input, std::string& output,
                   encoding base = encoding::base_unknown);

std::size_t decoded_size(std::string_view input,
                         encoding base = encoding::base_unknown);

}  // namespace multibase
