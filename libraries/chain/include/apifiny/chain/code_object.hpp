#pragma once
#include <apifiny/chain/database_utils.hpp>
#include <boost/tuple/tuple_io.hpp>

#include "multi_index_includes.hpp"

namespace apifiny { namespace chain {

   class code_object : public chainbase::object<code_object_type, code_object> {
      OBJECT_CTOR(code_object, (code))

      id_type      id;
      digest_type  code_hash; //< code_hash should not be changed within a chainbase modifier lambda
      shared_blob  code;
      uint64_t     code_ref_count;
      uint32_t     first_block_used;
      uint8_t      vm_type = 0; //< vm_type should not be changed within a chainbase modifier lambda
      uint8_t      vm_version = 0; //< vm_version should not be changed within a chainbase modifier lambda
   };

   struct by_code_hash;
   using code_index = chainbase::shared_multi_index_container<
      code_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<code_object, code_object::id_type, &code_object::id>>,
         ordered_unique<tag<by_code_hash>,
            composite_key< code_object,
               member<code_object, digest_type, &code_object::code_hash>,
               member<code_object, uint8_t,     &code_object::vm_type>,
               member<code_object, uint8_t,     &code_object::vm_version>
            >
         >
      >
   >;

} } // apifiny::chain

CHAINBASE_SET_INDEX_TYPE(apifiny::chain::code_object, apifiny::chain::code_index)

FC_REFLECT(apifiny::chain::code_object, (code_hash)(code)(code_ref_count)(first_block_used)(vm_type)(vm_version))
