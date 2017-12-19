/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eos/utilities/exception_macros.hpp>

namespace eosio { namespace types {

   FC_DECLARE_EXCEPTION(type_exception, 4000000, "type exception")
   FC_DECLARE_DERIVED_EXCEPTION(unknown_type_exception, type_exception,
                                4010000, "Could not find requested type")
   FC_DECLARE_DERIVED_EXCEPTION(duplicate_type_exception, type_exception,
                                4020000, "Requested type already exists")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_type_name_exception, type_exception,
                                4030000, "Requested type name is invalid")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_field_name_exception, type_exception,
                                4040000, "Requested field name is invalid")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_schema_exception, type_exception,
                                4050000, "Schema is invalid")

} } // eosio::types
