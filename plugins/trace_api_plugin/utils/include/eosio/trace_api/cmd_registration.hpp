#pragma once

#include <string>
#include <functional>
#include <vector>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

namespace eosio::trace_api {
   /**
    * passes global opts and then the remaining command line
    */
   using command_fn = std::function<int( const bpo::variables_map&, const std::vector<std::string>&)>;

   /**
    * basic initialization time linked list for registering commands
    */
   struct command_registration {
      command_registration( std::string name, std::string slug, command_fn func )
      :name(std::move(name))
      ,slug(std::move(slug))
      ,func(std::move(func))
      {
         _next = _list;
         _list = this;
      }

      std::string name;
      std::string slug;
      command_fn func;
      command_registration *_next;

      static command_registration * _list;
   };
}