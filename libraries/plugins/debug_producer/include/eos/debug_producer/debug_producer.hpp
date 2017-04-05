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

#include <eos/app/plugin.hpp>
#include <eos/chain/database.hpp>
#include <eos/chain/protocol/types.hpp>

#include <fc/thread/future.hpp>
#include <fc/container/flat.hpp>

namespace eos { namespace debug_producer_plugin {

class debug_producer_plugin : public eos::app::plugin {
public:
   ~debug_producer_plugin();

   std::string plugin_name()const override;

   virtual void plugin_set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   void set_json_object_stream( const std::string& filename );
   void flush_json_object_stream();

private:

   void on_applied_block( const eos::chain::signed_block& b );

   boost::program_options::variables_map _options;

   std::map<chain::public_key_type, fc::ecc::private_key> _private_keys;

   std::shared_ptr< std::ofstream > _json_object_stream;
   boost::signals2::scoped_connection _applied_block_conn;
   boost::signals2::scoped_connection _changed_objects_conn;
   boost::signals2::scoped_connection _removed_objects_conn;
};

} } //eos::debug_producer_plugin
