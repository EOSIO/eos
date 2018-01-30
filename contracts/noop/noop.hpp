/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eoslib/eos.hpp>
#include <eoslib/dispatcher.hpp>
#include <eoslib/types.hpp>
#include <eoslib/string.hpp>

using namespace eosio;

namespace noop {
   /**
      noop contract
      All it does is require sender authorization.
      Actions: anyaction and a template staticany.
    */


   class noop {
      public:
         
         ACTION(N(noop), anyaction) {
            anyaction() { }
            anyaction(account_name f, const string& t, const string& d): from(f), type(t), data(d) { }
            
            account_name from;
            string type;
            string data;
            
            EOSLIB_SERIALIZE(anyaction, (from)(type)(data))
         };

         template <typename T, uint64_t Name>
         struct staticany: action_meta<N(noop), Name> {
            account_name from;
            T data;
               
            EOSLIB_SERIALIZE(staticany, (from)(data))
         };

         static void on(const anyaction& act)
         {
            require_auth(act.from);
         }

         template <typename T, uint64_t Name>
         static void on(const staticany<T, Name>& act)
         {
            require_auth(act.from);
         }

   };
} /// noop
