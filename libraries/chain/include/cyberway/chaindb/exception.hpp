#pragma once

#include <eosio/chain/exceptions.hpp>

#define CYBERWAY_ASSERT(expr, exc_type, FORMAT, ...) EOS_ASSERT(expr, exc_type, FORMAT, __VA_ARGS__)

namespace cyberway { namespace chaindb {

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_exception, eosio::chain::chain_exception,
                                 3700000, "CyberWay ChainDB exception")

    /**
     *  chaindb_exception
     *   |- chaindb_internal_exception
     *   |- chaindb_abi_exception
     *   |- chaindb_contract_exception
     */

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

        FC_DECLARE_DERIVED_EXCEPTION(driver_out_of_range_exception, chaindb_internal_exception,
                                     3710005, "ChainDB driver object try to locate object in out of range")

        FC_DECLARE_DERIVED_EXCEPTION(driver_invalid_cursor_exception, chaindb_internal_exception,
                                     3710006, "ChainDB driver cursor doesn't exist")

        FC_DECLARE_DERIVED_EXCEPTION(driver_absent_object_exception, chaindb_internal_exception,
                                     3710007, "ChainDB driver can't find object")

        FC_DECLARE_DERIVED_EXCEPTION(driver_insert_exception, chaindb_internal_exception,
                                     3710008, "ChainDB driver fail to insert object")

        FC_DECLARE_DERIVED_EXCEPTION(driver_update_exception, chaindb_internal_exception,
                                     3710009, "ChainDB driver fail to update object")

        FC_DECLARE_DERIVED_EXCEPTION(driver_delete_exception, chaindb_internal_exception,
                                     3710010, "ChainDB driver fail to delete object")

        FC_DECLARE_DERIVED_EXCEPTION(session_exception, chaindb_internal_exception,
                                     3710011, "ChainDB session failed")

        FC_DECLARE_DERIVED_EXCEPTION(index_exception, chaindb_internal_exception,
                                     3710012, "ChainDB multi_index failed")

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

    FC_DECLARE_DERIVED_EXCEPTION(chaindb_contract_exception, chaindb_exception,
                                 3730000, "ChainDB contract exception")

        FC_DECLARE_DERIVED_EXCEPTION(invalid_data_size_exception, chaindb_contract_exception,
                                     3730001, "Requested data with wrong size")

        FC_DECLARE_DERIVED_EXCEPTION(reserved_field_name_exception, chaindb_contract_exception,
                                     3730002, "Table contains reserved field name")

        FC_DECLARE_DERIVED_EXCEPTION(primary_key_exception, chaindb_contract_exception,
                                     3730003, "Table has wrong type of primary key")

} } // namespace cyberway::chaindb