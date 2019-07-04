/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/types.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {
      /**
       *  @brief tracks the version of the application data stored in the database
       *  @ingroup object
       *
       *  the in-memory database expects that binay structures of data do not shift between executions.  Some
       *  upgrades will bump this version to indicate that the expectations of the binary application data
       *  have changed.  When it is safe to directly use an older version that will be allowed though cases
       *  where this is possible may be rare.
       */
      class database_header_object : public chainbase::object<database_header_object_type, database_header_object>
      {
         OBJECT_CTOR(database_header_object)

         /**
          *  VERSION HISTORY
          *   - 0 : implied version when this header is absent
          *   - 1 : initial version, prior to this no `database_header_object` existed in the shared memory file but
          *         no changes to its format were made so it can be safely added to existing databases
          *   - 2 : shared_authority now holds shared_key_weights & shared_public_keys
          */

         static constexpr uint32_t current_version            = 2;
         static constexpr uint32_t minimum_version            = 2;

         id_type        id;
         uint32_t       version = current_version;

         void validate() const {
            EOS_ASSERT(std::clamp(version, minimum_version, current_version) == version, bad_database_version_exception,
                       "state database version is incompatible, please restore from a compatible snapshot or replay!",
                       ("version", version)("minimum_version", minimum_version)("maximum_version", current_version));
         }
      };

      struct by_block_id;
      using database_header_multi_index = chainbase::shared_multi_index_container<
            database_header_object,
            indexed_by<
                  ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(database_header_object, database_header_object::id_type, id)>
             >
      >;

   } }

CHAINBASE_SET_INDEX_TYPE(eosio::chain::database_header_object, eosio::chain::database_header_multi_index)

FC_REFLECT( eosio::chain::database_header_object, (version) )
