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
#include <eos/debug_producer/debug_producer.hpp>

#include <eos/chain/database.hpp>
#include <eos/chain/producer_object.hpp>

#include <eos/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

using namespace eos::debug_producer_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

debug_producer_plugin::~debug_producer_plugin() {}

void debug_producer_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));
   command_line_options.add_options()
         ("private-key", bpo::value<vector<string>>()->composing()->multitoken()->
          DEFAULT_VALUE_VECTOR(std::make_pair(chain::public_key_type(default_priv_key.get_public_key()), eos::utilities::key_to_wif(default_priv_key))),
          "Tuple of [PublicKey, WIF private key] (may specify multiple times)");
   config_file_options.add(command_line_options);
}

std::string debug_producer_plugin::plugin_name()const
{
   return "debug_producer";
}

void debug_producer_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("debug_producer plugin:  plugin_initialize() begin");
   _options = &options;

   if( options.count("private-key") )
   {
      const std::vector<std::string> key_id_to_wif_pair_strings = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
      {
         auto key_id_to_wif_pair = eos::app::dejsonify<std::pair<chain::public_key_type, std::string> >(key_id_to_wif_pair_string);
         idump((key_id_to_wif_pair));
         fc::optional<fc::ecc::private_key> private_key = eos::utilities::wif_to_key(key_id_to_wif_pair.second);
         if (!private_key)
         {
            // the key isn't in WIF format; see if they are still passing the old native private key format.  This is
            // just here to ease the transition, can be removed soon
            try
            {
               private_key = fc::variant(key_id_to_wif_pair.second).as<fc::ecc::private_key>();
            }
            catch (const fc::exception&)
            {
               FC_THROW("Invalid WIF-format private key ${key_string}", ("key_string", key_id_to_wif_pair.second));
            }
         }
         _private_keys[key_id_to_wif_pair.first] = *private_key;
      }
   }
   ilog("debug_producer plugin:  plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void debug_producer_plugin::plugin_startup()
{
   ilog("debug_producer_plugin::plugin_startup() begin");
   chain::database& db = database();

   // connect needed signals

   _applied_block_conn  = db.applied_block.connect([this](const eos::chain::signed_block& b){ on_applied_block(b); });

   return;
}

void debug_producer_plugin::on_applied_block( const eos::chain::signed_block& b )
{
   if( _json_object_stream )
   {
      (*_json_object_stream) << "{\"bn\":" << fc::to_string( b.block_num() ) << "}\n";
   }
}

void debug_producer_plugin::set_json_object_stream( const std::string& filename )
{
   if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }
   _json_object_stream = std::make_shared< std::ofstream >( filename );
}

void debug_producer_plugin::flush_json_object_stream()
{
   if( _json_object_stream )
      _json_object_stream->flush();
}

void debug_producer_plugin::plugin_shutdown()
{
   if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }
   return;
}
