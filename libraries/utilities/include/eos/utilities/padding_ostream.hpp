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
#pragma once

namespace eos { namespace utilities {

template<size_t BlockSize=16, char PaddingChar=' '>
class padding_ostream : public fc::buffered_ostream {
public:
   padding_ostream( fc::ostream_ptr o, size_t bufsize = 4096 ) : buffered_ostream(o, bufsize) {}
   virtual ~padding_ostream() {}

   virtual size_t writesome( const char* buffer, size_t len ) {
      auto out = buffered_ostream::writesome(buffer, len);
      bytes_out += out;
      bytes_out %= BlockSize;
      return out;
   }
   virtual size_t writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset ) {
      auto out = buffered_ostream::writesome(buf, len, offset);
      bytes_out += out;
      bytes_out %= BlockSize;
      return out;
   }
   virtual void flush() {
      static const char pad = PaddingChar;
      while( bytes_out % BlockSize )
         writesome(&pad, 1);
      buffered_ostream::flush();
   }

private:
   size_t bytes_out = 0;
};

} } //eos::utilities

