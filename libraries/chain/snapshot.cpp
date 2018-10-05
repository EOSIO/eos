#include <eosio/chain/snapshot.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

variant_snapshot_writer::variant_snapshot_writer()
: snapshot(fc::mutable_variant_object()("sections", fc::variants())("version", current_snapshot_version ))
{

}

void variant_snapshot_writer::write_start_section( const std::string& section_name ) {
   current_rows.clear();
   current_section_name = section_name;
}

void variant_snapshot_writer::write_row( const detail::abstract_snapshot_row_writer& row_writer ) {
   current_rows.emplace_back(row_writer.to_variant());
}

void variant_snapshot_writer::write_end_section( ) {
   snapshot["sections"].get_array().emplace_back(fc::mutable_variant_object()("name", std::move(current_section_name))("rows", std::move(current_rows)));
}

fc::variant variant_snapshot_writer::finalize() {
   fc::variant result = std::move(snapshot);
   return result;
}

variant_snapshot_reader::variant_snapshot_reader(const fc::variant& snapshot)
:snapshot(snapshot)
,cur_row(0)
{
}

void variant_snapshot_reader::validate() const {
   EOS_ASSERT(snapshot.is_object(), snapshot_validation_exception,
         "Variant snapshot is not an object");
   const fc::variant_object& o = snapshot.get_object();

   EOS_ASSERT(o.contains("version"), snapshot_validation_exception,
         "Variant snapshot has no version");

   const auto& version = o["version"];
   EOS_ASSERT(version.is_integer(), snapshot_validation_exception,
         "Variant snapshot version is not an integer");

   EOS_ASSERT(version.as_uint64() == (uint64_t)current_snapshot_version, snapshot_validation_exception,
         "Variant snapshot is an unsuppored version.  Expected : ${expected}, Got: ${actual}",
         ("expected", current_snapshot_version)("actual",o["version"].as_uint64()));

   EOS_ASSERT(o.contains("sections"), snapshot_validation_exception,
         "Variant snapshot has no sections");

   const auto& sections = o["sections"];
   EOS_ASSERT(sections.is_array(), snapshot_validation_exception, "Variant snapshot sections is not an array");

   const auto& section_array = sections.get_array();
   for( const auto& section: section_array ) {
      EOS_ASSERT(section.is_object(), snapshot_validation_exception, "Variant snapshot section is not an object");

      const auto& so = section.get_object();
      EOS_ASSERT(so.contains("name"), snapshot_validation_exception,
            "Variant snapshot section has no name");

      EOS_ASSERT(so["name"].is_string(), snapshot_validation_exception,
                 "Variant snapshot section name is not a string");

      EOS_ASSERT(so.contains("rows"), snapshot_validation_exception,
                 "Variant snapshot section has no rows");

      EOS_ASSERT(so["rows"].is_array(), snapshot_validation_exception,
                 "Variant snapshot section rows is not an array");
   }
}

void variant_snapshot_reader::set_section( const string& section_name ) {
   const auto& sections = snapshot["sections"].get_array();
   for( const auto& section: sections ) {
      if (section["name"].as_string() == section_name) {
         cur_section = &section.get_object();
         break;
      }
   }
}

bool variant_snapshot_reader::read_row( detail::abstract_snapshot_row_reader& row_reader ) {
   const auto& rows = (*cur_section)["rows"].get_array();
   row_reader.provide(rows.at(cur_row++));
   return cur_row < rows.size();
}

bool variant_snapshot_reader::empty ( ) {
   const auto& rows = (*cur_section)["rows"].get_array();
   return rows.empty();
}

void variant_snapshot_reader::clear_section() {
   cur_section = nullptr;
   cur_row = 0;
}


}}