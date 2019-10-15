#pragma once

#include <multibase/encoding.h>
#include <memory>
#include <string>
#include <string_view>

namespace multibase {

class codec {
 public:
  explicit codec(encoding base);

  encoding base() const;

  /** Determine whether the input is a valid encoding in this base */
  bool is_valid(std::string_view input, bool include_encoding = true);

  std::string encode(std::string_view input, bool include_encoding = true);

  /** Encode the input, writing the result to the user-supplied buffer,
   * optionally including the encoding type in the output
   * @return Number of bytes written */
  std::size_t encode(std::string_view input, std::string& output,
                     bool include_encoding = true);

  std::size_t encoded_size(std::string_view input,
                           bool include_encoding = true);

  std::string decode(std::string_view input);

  virtual std::size_t decode(std::string_view input, std::string& output);

  std::size_t decoded_size(std::string_view input);

  class impl;

 private:
  impl* pImpl;
};

}  // namespace multibase
