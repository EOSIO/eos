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
#include <fstream>
#include <eos/chain/protocol/block.hpp>

namespace eos { namespace chain {
   class block_database 
   {
      public:
         void open( const fc::path& dbdir );
         bool is_open()const;
         void flush();
         void close();

         void store( const block_id_type& id, const signed_block& b );
         void remove( const block_id_type& id );

         bool                   contains( const block_id_type& id )const;
         block_id_type          fetch_block_id( uint32_t block_num )const;
         optional<signed_block> fetch_optional( const block_id_type& id )const;
         optional<signed_block> fetch_by_number( uint32_t block_num )const;
         optional<signed_block> last()const;
         optional<block_id_type> last_id()const;
      private:
         mutable std::fstream _blocks;
         mutable std::fstream _block_num_to_pos;
   };
} }
