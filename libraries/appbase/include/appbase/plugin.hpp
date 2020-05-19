#pragma once
#include <boost/program_options.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <string>
#include <vector>
#include <map>

#define APPBASE_PLUGIN_REQUIRES_VISIT( r, visitor, elem ) \
  visitor( appbase::app().register_plugin<elem>() ); 

#define APPBASE_PLUGIN_REQUIRES( PLUGINS )                               \
   template<typename Lambda>                                           \
   void plugin_requires( Lambda&& l ) {                                \
      BOOST_PP_SEQ_FOR_EACH( APPBASE_PLUGIN_REQUIRES_VISIT, l, PLUGINS ) \
   }

namespace appbase {

   using boost::program_options::options_description;
   using boost::program_options::variables_map;
   using std::string;
   using std::vector;
   using std::map;

   class application;
   application& app();

   class abstract_plugin {
      public:
         enum state {
            registered, ///< the plugin is constructed but doesn't do anything
            initialized, ///< the plugin has initialized any state required but is idle
            started, ///< the plugin is actively running
            stopped ///< the plugin is no longer running
         };

         virtual ~abstract_plugin(){}
         virtual state get_state()const = 0;
         virtual const std::string& name()const  = 0;
         virtual void set_program_options( options_description& cli, options_description& cfg ) = 0;
         virtual void initialize(const variables_map& options) = 0;
         virtual void handle_sighup() = 0;
         virtual void startup() = 0;
         virtual void shutdown() = 0;
   };

   template<typename Impl>
   class plugin;
}
