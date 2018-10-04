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
      using row_type = std::decay_t<T>;
      using value_type = const row_type&;
   };

   template<typename T>
   auto to_snapshot_row( const T& value ) -> typename snapshot_row_traits<T>::value_type {
      return value;
   };

   template<typename T>
   auto from_snapshot_row( typename snapshot_row_traits<T>::value_type&& row, T& value ) {
      value = row;
   }

   namespace detail {
      struct abstract_snapshot_row_writer {
         virtual void write(std::ostream& out) const = 0;
         virtual variant to_variant() const = 0;
      };

      template<typename T>
      struct snapshot_row_writer : abstract_snapshot_row_writer {
         explicit snapshot_row_writer( const T& data )
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
         class section_writer {
            public:
               template<typename T>
               void add_row( const T& row ) {
                  _writer.write_row(detail::make_row_writer(to_snapshot_row(row)));
               }

            private:
               friend class snapshot_writer;
               section_writer(snapshot_writer& writer)
               :_writer(writer)
               {

               }
               snapshot_writer& _writer;
         };

         template<typename T, typename F>
         void write_section(F f) {
            write_start_section(snapshot_section_traits<T>::section_name());
            auto section = section_writer(*this);
            f(section);
            write_end_section();
         }

      virtual ~snapshot_writer(){};

      protected:
         virtual void write_start_section( const std::string& section_name ) = 0;
         virtual void write_row( const detail::abstract_snapshot_row_writer& row_writer ) = 0;
         virtual void write_end_section() = 0;
   };

   using snapshot_writer_ptr = std::shared_ptr<snapshot_writer>;

   namespace detail {
      struct abstract_snapshot_row_reader {
         virtual void provide(std::istream& in) const = 0;
         virtual void provide(const fc::variant&) const = 0;
      };

      template<typename T>
      struct snapshot_row_reader : abstract_snapshot_row_reader {
         explicit snapshot_row_reader( T& data )
         :data(data) {}

         void provide(std::istream& in) const override {
            fc::raw::unpack(in, data);
         }

         void provide(const fc::variant& var) const override {
            fc::from_variant(var, data);
         }

         T& data;
      };

      template<typename T>
      snapshot_row_reader<T> make_row_reader( T& data ) {
         return snapshot_row_reader<T>(data);
      }
   }

   class snapshot_reader {
      public:
         class section_reader {
            public:
               template<typename T>
               auto read_row( T& out ) -> std::enable_if_t<std::is_same<std::decay_t<T>, typename snapshot_row_traits<T>::row_type>::value,bool> {
                  auto reader = detail::make_row_reader(out);
                  return _reader.read_row(reader);
               }

               template<typename T>
               auto read_row( T& out ) -> std::enable_if_t<!std::is_same<std::decay_t<T>, typename snapshot_row_traits<T>::row_type>::value,bool> {
                  auto temp = typename snapshot_row_traits<T>::row_type();
                  auto reader = detail::make_row_reader(temp);
                  bool result = _reader.read_row(reader);
                  from_snapshot_row(std::move(temp), out);
                  return result;
               }

               bool empty() {
                  return _reader.empty();
               }

            private:
               friend class snapshot_reader;
               section_reader(snapshot_reader& _reader)
               :_reader(_reader)
               {}

               snapshot_reader& _reader;

         };

      template<typename T, typename F>
      void read_section(F f) {
         set_section(snapshot_section_traits<T>::section_name());
         auto section = section_reader(*this);
         f(section);
         clear_section();
      }

      virtual ~snapshot_reader(){};

      protected:
         virtual void set_section( const std::string& section_name ) = 0;
         virtual bool read_row( detail::abstract_snapshot_row_reader& row_reader ) = 0;
         virtual bool empty( ) = 0;
         virtual void clear_section() = 0;
   };

   using snapshot_reader_ptr = std::shared_ptr<snapshot_reader>;

   class variant_snapshot_writer : public snapshot_writer {
      public:
         variant_snapshot_writer()
         : snapshot(fc::mutable_variant_object()("sections", fc::variants()))
         {

         }

         void write_start_section( const std::string& section_name ) override {
            current_rows.clear();
            current_section_name = section_name;
         }

         void write_row( const detail::abstract_snapshot_row_writer& row_writer ) override {
            current_rows.emplace_back(row_writer.to_variant());
         }

         void write_end_section( ) override {
            snapshot["sections"].get_array().emplace_back(fc::mutable_variant_object()("name", std::move(current_section_name))("rows", std::move(current_rows)));
         }

         fc::variant finalize() {
            fc::variant result = std::move(snapshot);
            return result;
         }
      private:
         fc::mutable_variant_object snapshot;
         std::string current_section_name;
         fc::variants current_rows;
   };

   class variant_snapshot_reader : public snapshot_reader {
      public:
         variant_snapshot_reader(const fc::variant& snapshot)
         :snapshot(snapshot)
         ,cur_row(0)
         {

         }

         void set_section( const string& section_name ) override {
            const auto& sections = snapshot["sections"].get_array();
            for( const auto& section: sections ) {
               if (section["name"].as_string() == section_name) {
                  cur_section = &section.get_object();
                  break;
               }
            }
         }

         bool read_row( detail::abstract_snapshot_row_reader& row_reader ) override {
            const auto& rows = (*cur_section)["rows"].get_array();
            row_reader.provide(rows.at(cur_row++));
            return cur_row < rows.size();
         }

         bool empty ( ) override {
            const auto& rows = (*cur_section)["rows"].get_array();
            return rows.empty();
         }

         void clear_section() override {
            cur_section = nullptr;
            cur_row = 0;
         }

      private:
         const fc::variant& snapshot;
         const fc::variant_object* cur_section;
         int cur_row;
   };

}}
