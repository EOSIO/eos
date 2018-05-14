/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

namespace eosio { namespace chain {


   class block_context {
      public:

         enum block_status {
            dpos_irreversible = 0, ///< this block has already been applied before by this node and is considered irreversible by DPOS standards
            bft_irreversible  = 1, ///< this block has already been applied before by this node and is considered irreversible by BFT standards (but not yet DPOS standards)
            validated_block   = 2, ///< this is a complete block signed by a valid producer and has been previously applied by this node and therefore validated but it is not yet irreversible
            completed_block   = 3, ///< this is a complete block signed by a valid producer but is not yet irreversible nor has it yet been applied by this node
            producing_block   = 4, ///< this is an incomplete block that is being produced by a valid producer for their time slot and will be signed by them after the block is completed
            speculative_block   = 5  ///< this is an incomplete block that is only being speculatively produced by the node (whether it is the node of an active producer or not)
         };

         block_status status = speculative_block;
         bool is_active_producer = false; ///< whether the node applying the block is an active producer (this further modulates behavior when the block status is completed_block)
         
   };

} }
