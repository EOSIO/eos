/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <eos/utilities/string_escape.hpp>
#include <sstream>

namespace eos { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input)
  {
    std::ostringstream escaped_string;
    escaped_string << "\"";
    for (unsigned i = 0; i < input.size(); ++i)
    {
      switch (input[i])
      {
      case '\a': 
        escaped_string << "\\a";
        break;
      case '\b': 
        escaped_string << "\\b";
        break;
      case '\t': 
        escaped_string << "\\t";
        break;
      case '\n': 
        escaped_string << "\\n";
        break;
      case '\v': 
        escaped_string << "\\v";
        break;
      case '\f': 
        escaped_string << "\\f";
        break;
      case '\r': 
        escaped_string << "\\r";
        break;
      case '\\': 
        escaped_string << "\\\\";
        break;
      case '\"': 
        escaped_string << "\\\"";
        break;
      default:
        escaped_string << input[i];
      }
    }
    escaped_string << "\"";
    return escaped_string.str();
  }

} } // end namespace eos::utilities

