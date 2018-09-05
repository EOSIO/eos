/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/variant_object.hpp>
#include <boost/core/demangle.hpp>


namespace eosio { namespace chain {

   template<typename T>
   struct snapshot_section_traits {
      static std::string section_name() {
         return boost::core::demangle(typeid(T).name());
      }
   };


   template<typename T>
   struct snapshot_row_traits {
      using row_type = T;
      using value_type = const T&;
   };

   template<typename T>
   auto to_snapshot_row( const T& value ) -> typename snapshot_row_traits<T>::value_type {
      return value;
   };

   class abstract_snapshot_writer {
      public:
         template<typename T>
         void start_section() {
            start_named_section(snapshot_section_traits<T>::section_name());
         }

         template<typename T>
         void add_row( const T& row ) {
            fc::variant vrow;
            fc::to_variant(to_snapshot_row(row), vrow);
            add_variant_row(std::move(vrow));
         }

         void end_section( ) {
            end_named_section();
         }

      protected:
         virtual void start_named_section( const std::string& section_name ) = 0;
         virtual void add_variant_row( fc::variant&& row ) = 0;
         virtual void end_named_section() = 0;
   };

}}
