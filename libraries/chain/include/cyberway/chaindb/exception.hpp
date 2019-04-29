#pragma once

#include <eosio/chain/exceptions.hpp>

#define CYBERWAY_ASSERT(expr, exc_type, FORMAT, ...) EOS_ASSERT(expr, exc_type, FORMAT, __VA_ARGS__)

namespace cyberway { namespace chaindb {

    /**
     *  chaindb_midx_exception -
     *
     *
     *   |- chaindb_midx_logic_exception
     *   |- chaindb_midx_pk_exception
     *   |- chaindb_midx_find_exception
     */

    /**
     *  chain
     */
    FC_DECLARE_EXCEPTION(chaindb_midx_exception,
                         3800000, "CyberWay ChainDB multi-index exception" )

        FC_DECLARE_DERIVED_EXCEPTION(chaindb_midx_logic_exception, chaindb_midx_exception,
                                     3800001, "Internal unknown exception in ChainDB multi-index")

        FC_DECLARE_DERIVED_EXCEPTION(chaindb_midx_pk_exception, chaindb_midx_exception,
                                     3800002, "Bad primary key in ChainDB multi-index")

        FC_DECLARE_DERIVED_EXCEPTION(chaindb_midx_find_exception, chaindb_midx_exception,
                                     3800003, "Unable to find key in ChainDB multi-index")

    /**
     *  chaindb_exception
     *   |- chaindb_internal_exception
     *   |- chaindb_abi_exception
     *   |- chaindb_contract_exception
     *   |- chaindb_object_exception
     */

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_exception, eosio::chain::chain_exception,
                                 3700000, "CyberWay ChainDB exception")

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_internal_exception, chaindb_exception,
                                 3710000, "Internal ChainDB exception")

        FC_DECLARE_DERIVED_EXCEPTION(unknown_connection_type_exception, chaindb_internal_exception,
                                     3710001, "Invalid type of ChainDB")

        FC_DECLARE_DERIVED_EXCEPTION(broken_driver_exception, chaindb_internal_exception,
                                     3710002, "Broken ChainDB driver interface")

        FC_DECLARE_DERIVED_EXCEPTION(driver_primary_key_exception, chaindb_internal_exception,
                                     3710003, "ChainDB driver returns bad primary key")

        FC_DECLARE_DERIVED_EXCEPTION(driver_absent_field_exception, chaindb_internal_exception,
                                     3710004, "ChainDB driver object without required key")

        FC_DECLARE_DERIVED_EXCEPTION(driver_wrong_field_type_exception, chaindb_internal_exception,
                                     3710005, "ChainDB driver object with wrong field type")

        FC_DECLARE_DERIVED_EXCEPTION(driver_wrong_field_name_exception, chaindb_internal_exception,
                                     3710006, "ChainDB driver object with wrong field name")

        FC_DECLARE_DERIVED_EXCEPTION(driver_out_of_range_exception, chaindb_internal_exception,
                                     3710007, "ChainDB driver object try to locate object in out of range")

        FC_DECLARE_DERIVED_EXCEPTION(driver_invalid_cursor_exception, chaindb_internal_exception,
                                     3710008, "ChainDB driver cursor doesn't exist")

        FC_DECLARE_DERIVED_EXCEPTION(driver_absent_object_exception, chaindb_internal_exception,
                                     3710009, "ChainDB driver can't find object")

        FC_DECLARE_DERIVED_EXCEPTION(session_exception, chaindb_internal_exception,
                                     3710010, "ChainDB session failed")

        FC_DECLARE_DERIVED_EXCEPTION(index_exception, chaindb_internal_exception,
                                     3710011, "ChainDB multi_index failed")

        FC_DECLARE_DERIVED_EXCEPTION(driver_has_unapplied_changes_exception, chaindb_internal_exception,
                                     3710012, "ChainDB driver has unapplied changes")

        FC_DECLARE_DERIVED_EXCEPTION(driver_write_exception, chaindb_internal_exception,
                                     3710013, "ChainDB driver write changes")

        FC_DECLARE_DERIVED_EXCEPTION(driver_duplicate_exception, chaindb_internal_exception,
                                     3710014, "ChainDB driver duplicate unique records")

        FC_DECLARE_DERIVED_EXCEPTION(driver_open_exception, chaindb_internal_exception,
                                     3710015, "ChainDB driver fail to open database")

        FC_DECLARE_DERIVED_EXCEPTION(driver_opened_cursors_exception, chaindb_internal_exception,
                                     3710016, "ChainDB driver has opened cursors")

        FC_DECLARE_DERIVED_EXCEPTION(driver_unsupported_operation_exception, chaindb_internal_exception,
                                     3710017, "ChainDB driver does not support operation")

        FC_DECLARE_DERIVED_EXCEPTION(driver_wrong_object_exception, chaindb_internal_exception,
                                     3710018, "ChainDB driver return wrong object")

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_abi_exception, chaindb_exception,
                                 3720000, "ChainDB ABI exception")

        FC_DECLARE_DERIVED_EXCEPTION(unknown_table_exception, chaindb_abi_exception,
                                     3720001, "Unknown table")

        FC_DECLARE_DERIVED_EXCEPTION(unknown_index_exception, chaindb_abi_exception,
                                     3720002, "Unknown index")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_abi_store_type_exception, chaindb_abi_exception,
                                     3720003, "Invalid type for serialization")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_primary_key_exception, chaindb_abi_exception,
                                     3720004, "Invalid primary key information")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_index_description_exception, chaindb_abi_exception,
                                     3720005, "Invalid index description")

        FC_DECLARE_DERIVED_EXCEPTION(no_abi_exception, chaindb_abi_exception,
                                     3720006, "Account doesn't have an ABI description")

        FC_DECLARE_DERIVED_EXCEPTION(table_has_rows_exception, chaindb_abi_exception,
                                     3720007, "Table has rows, it can't be changed")

        FC_DECLARE_DERIVED_EXCEPTION(max_table_count_exception, chaindb_abi_exception,
                                     3720008, "Maximum number of tables is exceeded")

        FC_DECLARE_DERIVED_EXCEPTION(max_index_count_exception, chaindb_abi_exception,
                                     3720009, "Maximum number of indexes per table is exceeded")

        FC_DECLARE_DERIVED_EXCEPTION(unique_index_name_exception, chaindb_abi_exception,
                                     3720010, "Index name in table scope should be unique")

        FC_DECLARE_DERIVED_EXCEPTION(unique_field_name_exception, chaindb_abi_exception,
                                     3720011, "Field name in index scope should be unique")

        FC_DECLARE_DERIVED_EXCEPTION(array_field_exception, chaindb_abi_exception,
                                     3720012, "Array field can't be used for index")

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_contract_exception, chaindb_exception,
                                 3730000, "ChainDB contract exception")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_data_size_exception, chaindb_contract_exception,
                                     3730001, "Requested data with wrong size")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_data_exception, chaindb_contract_exception,
                                     3730002, "Requested data is invalid")

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_object_exception, chaindb_exception,
                                 3740000, "ChainDB object exception")

        FC_DECLARE_DERIVED_EXCEPTION(primary_key_exception, chaindb_object_exception,
                                     3740001, "Object has wrong primary key")

        FC_DECLARE_DERIVED_EXCEPTION(reserved_field_exception, chaindb_object_exception,
                                     3740002, "Object has reserved field name")

        FC_DECLARE_DERIVED_EXCEPTION(object_exception, chaindb_object_exception,
                                     3740003, "Object has reserved field name")

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_cache_exception, chaindb_exception,
                                 3750000, "ChainDB cache exception")

        FC_DECLARE_DERIVED_EXCEPTION(cache_usage_exception, chaindb_cache_exception,
                                     3750001, "Object size exception")


} } // namespace cyberway::chaindb
