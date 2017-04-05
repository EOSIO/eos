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

#include <eos/app/application.hpp>

#include <boost/program_options.hpp>
#include <fc/io/json.hpp>

namespace eos { namespace app {

class abstract_plugin
{
   public:
      virtual ~abstract_plugin(){}
      virtual std::string plugin_name()const = 0;

      /**
       * @brief Perform early startup routines and register plugin indexes, callbacks, etc.
       *
       * Plugins MUST supply a method initialize() which will be called early in the application startup. This method
       * should contain early setup code such as initializing variables, adding indexes to the database, registering
       * callback methods from the database, adding APIs, etc., as well as applying any options in the @ref options map
       *
       * This method is called BEFORE the database is open, therefore any routines which require any chain state MUST
       * NOT be called by this method. These routines should be performed in startup() instead.
       *
       * @param options The options passed to the application, via configuration files or command line
       */
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) = 0;

      /**
       * @brief Begin normal runtime operations
       *
       * Plugins MUST supply a method startup() which will be called at the end of application startup. This method
       * should contain code which schedules any tasks, or requires chain state.
       */
      virtual void plugin_startup() = 0;

      /**
       * @brief Cleanly shut down the plugin.
       *
       * This is called to request a clean shutdown (e.g. due to SIGINT or SIGTERM).
       */
      virtual void plugin_shutdown() = 0;

      /**
       * @brief Register the application instance with the plugin.
       *
       * This is called by the framework to set the application.
       */
      virtual void plugin_set_app( application* a ) = 0;

      /**
       * @brief Fill in command line parameters used by the plugin.
       *
       * @param command_line_options All options this plugin supports taking on the command-line
       * @param config_file_options All options this plugin supports storing in a configuration file
       *
       * This method populates its arguments with any
       * command-line and configuration file options the plugin supports.
       * If a plugin does not need these options, it
       * may simply provide an empty implementation of this method.
       */
      virtual void plugin_set_program_options(
         boost::program_options::options_description& command_line_options,
         boost::program_options::options_description& config_file_options
         ) = 0;
};

/**
 * Provides basic default implementations of abstract_plugin functions.
 */

class plugin : public abstract_plugin
{
   public:
      plugin();
      virtual ~plugin() override;

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
      virtual void plugin_set_app( application* app ) override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& command_line_options,
         boost::program_options::options_description& config_file_options
         ) override;

      chain::database& database() { return *app().chain_database(); }
      application& app()const { assert(_app); return *_app; }
   protected:
      net::node& p2p_node() { return *app().p2p_node(); }

   private:
      application* _app = nullptr;
};

/// @group Some useful tools for boost::program_options arguments using vectors of JSON strings
/// @{
template<typename T>
T dejsonify(const string& s)
{
   return fc::json::from_string(s).as<T>();
}

#define DEFAULT_VALUE_VECTOR(value) default_value({fc::json::to_string(value)}, fc::json::to_string(value))
#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
      const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
      std::transform(ops.begin(), ops.end(), std::inserter(container, container.end()), &eos::app::dejsonify<type>); \
}
/// @}

} } //eos::app
