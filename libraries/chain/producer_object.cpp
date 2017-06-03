#include <eos/chain/producer_object.hpp>

namespace eos { namespace chain {

void producer_object::update_votes(ShareType new_votes, UInt128 current_virtual_time) {
   // Update virtual_position to reflect the producer's movement since virtual_last_update
   virtual_position += votes * (current_virtual_time - virtual_last_update_time);
}

} } // namespace eos::chain
