/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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

#include <memory>
#include <string>

#include <fc/api.hpp>
#include <fc/variant_object.hpp>

namespace eos { namespace app {
class application;
} }

namespace eos { namespace debug_producer {

namespace detail {
class debug_api_impl;
}

class debug_api
{
   public:
      debug_api( eos::app::application& app );

      /**
       * Push blocks from existing database.
       */
      void debug_push_blocks( std::string src_filename, uint32_t count );

      /**
       * Generate blocks locally.
       */
      void debug_generate_blocks( std::string debug_key, uint32_t count );

      /**
       * Directly manipulate database objects (will undo and re-apply last block with new changes post-applied).
       */
      void debug_update_object( fc::variant_object update );

      /**
       * Start a node with given initial path.
       */
      // not implemented
      //void start_node( std::string name, std::string initial_db_path );

      /**
       * Save the database to disk.
       */
      // not implemented
      //void save_db( std::string db_path );

      /**
       * Stream objects to file.  (Hint:  Create with mkfifo and pipe it to a script)
       */

      void debug_stream_json_objects( std::string filename );

      /**
       * Flush streaming file.
       */
      void debug_stream_json_objects_flush();

      std::shared_ptr< detail::debug_api_impl > my;
};

} }

FC_API(eos::debug_producer::debug_api,
       (debug_push_blocks)
       (debug_generate_blocks)
       (debug_update_object)
       (debug_stream_json_objects)
       (debug_stream_json_objects_flush)
     )
