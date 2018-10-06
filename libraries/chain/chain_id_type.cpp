/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */

#include <arisen/chain/chain_id_type.hpp>
#include <arisen/chain/exceptions.hpp>

namespace arisen { namespace chain {

   void chain_id_type::reflector_verify()const {
      RSN_ASSERT( *reinterpret_cast<const fc::sha256*>(this) != fc::sha256(), chain_id_type_exception, "chain_id_type cannot be zero" );
   }

} }  // namespace arisen::chain

namespace fc {

   void to_variant(const arisen::chain::chain_id_type& cid, fc::variant& v) {
      to_variant( static_cast<const fc::sha256&>(cid), v);
   }

   void from_variant(const fc::variant& v, arisen::chain::chain_id_type& cid) {
      from_variant( v, static_cast<fc::sha256&>(cid) );
   }

} // fc
