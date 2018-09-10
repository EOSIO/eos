/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/database_utils.hpp>
#include <fc/variant_object.hpp>
#include <boost/core/demangle.hpp>
#include <ostream>


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

   namespace detail {
      struct abstract_snapshot_row_writer {
         virtual void write(std::ostream& out) const = 0;
         virtual variant to_variant() const = 0;

      };
      template<typename T>
      struct snapshot_row_writer : abstract_snapshot_row_writer {
         explicit snapshot_row_writer( const T& data)
         :data(data) {}

         void write(std::ostream& out) const override {
            fc::raw::pack(out, data);
         }

         fc::variant to_variant() const override {
            variant var;
            fc::to_variant(data, var);
            return var;
         }

         const T& data;
      };

      template<typename T>
      snapshot_row_writer<T> make_row_writer( const T& data) {
         return snapshot_row_writer<T>(data);
      }
   }

   class snapshot_writer {
      public:
         template<typename T>
         void start_section() {
            write_section(snapshot_section_traits<T>::section_name());
         }

         template<typename T>
         void add_row( const T& row ) {
            write_row(detail::make_row_writer(to_snapshot_row(row)));
         }

         void end_section( ) {
            write_end_section();
         }

      protected:
         virtual void write_section( const std::string& section_name ) = 0;
         virtual void write_row( const detail::abstract_snapshot_row_writer& row_writer ) = 0;
         virtual void write_end_section() = 0;
   };

}}
