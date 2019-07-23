/**
 *  @copyright defined in eosio.cdt/LICENSE.txt
 */

#pragma once

#include <eosio/binary_extension.hpp>
#include <eosio/eosio.hpp>
#include <eosio/ignore.hpp>
#include <eosio/transaction.hpp>

namespace eosio {
   /**
    * @defgroup eosiomsig eosio.msig
    * @ingroup eosiocontracts
    * eosio.msig contract defines the structures and actions needed to manage the proposals and approvals on blockchain.
    * @{
    */
   class [[eosio::contract("eosio.msig")]] multisig : public contract {
      public:
         using contract::contract;

         /**
          * Create proposal
          * 
          * @details Creates a proposal containing one transaction.
          * Allows an account `proposer` to make a proposal `proposal_name` which has `requested`
          * permission levels expected to approve the proposal, and if approved by all expected 
          * permission levels then `trx` transaction can we executed by this proposal.
          * The `proposer` account is authorized and the `trx` transaction is verified if it was 
          * authorized by the provided keys and permissions, and if the proposal name doesn’t 
          * already exist; if all validations pass the `proposal_name` and `trx` trasanction are 
          * saved in the proposals table and the `requested` permission levels to the 
          * approvals table (for the `proposer` context).
          * Storage changes are billed to `proposer`.
          * 
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The name of the proposal (should be unique for proposer)
          * @param requested - Permission levels expected to approve the proposal
          * @param trx - Proposed transaction
          */
         [[eosio::action]]
         void propose(ignore<name> proposer, ignore<name> proposal_name,
               ignore<std::vector<permission_level>> requested, ignore<transaction> trx);
         /**
          * Approve proposal
          * 
          * @details Approves an existing proposal
          * Allows an account, the owner of `level` permission, to approve a proposal `proposal_name`
          * proposed by `proposer`. If the proposal's requested approval list contains the `level` 
          * permission then the `level` permission is moved from internal `requested_approvals` list to 
          * internal `provided_approvals` list of the proposal, thus persisting the approval for 
          * the `proposal_name` proposal.
          * Storage changes are billed to `proposer`.
          * 
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The name of the proposal (should be unique for proposer)
          * @param level - Permission level approving the transaction
          * @param proposal_hash - Transaction's checksum
          */
         [[eosio::action]]
         void approve( name proposer, name proposal_name, permission_level level,
                       const eosio::binary_extension<eosio::checksum256>& proposal_hash );
         /**
          * Revoke proposal
          * 
          * @details Revokes an existing proposal
          * This action is the reverse of the `approve` action: if all validations pass 
          * the `level` permission is erased from internal `provided_approvals` and added to the internal 
          * `requested_approvals` list, and thus un-approve or revoke the proposal.
          * 
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The name of the proposal (should be an existing proposal)
          * @param level - Permission level revoking approval for proposal
          */
         [[eosio::action]]
         void unapprove( name proposer, name proposal_name, permission_level level );
         /**
          * Cancel proposal
          * 
          * @details Cancels an existing proposal
          * 
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The name of the proposal (should be an existing proposal)
          * @param canceler - The account cancelling the proposal (only the proposer can cancel an unexpired transaction, and the canceler has to be different than the proposer)
          * 
          * Allows the `canceler` account to cancel the `proposal_name` proposal, created by a `proposer`, 
          * only after time has expired on the proposed transaction. It removes corresponding entries from  
          * internal proptable and from approval (or old approvals) tables as well.
          */
         [[eosio::action]]
         void cancel( name proposer, name proposal_name, name canceler );
         /**
          * Execute proposal
          * 
          * @details Allows an `executer` account to execute a proposal.
          * 
          * Preconditions: 
          * - `executer` has authorization, 
          * - `proposal_name` is found in the proposals table, 
          * - all requested approvals are received, 
          * - proposed transaction is not expired, 
          * - and approval accounts are not found in invalidations table. 
          * 
          * If all preconditions are met the transaction is executed as a deferred transaction,
          * and the proposal is erased from the proposals table.
          * 
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The name of the proposal (should be an existing proposal)
          * @param executer - The account executing the transaction
          */
         [[eosio::action]]
         void exec( name proposer, name proposal_name, name executer );
         /**
          * Invalidate proposal
          * 
          * @details Allows an `account` to invalidate itself, that is, its name is added to 
          * the invalidations table and this table will be cross referenced when exec is performed.
          * 
          * @param account - The account invalidating the transaction
          */
         [[eosio::action]]
         void invalidate( name account );

         using propose_action = eosio::action_wrapper<"propose"_n, &multisig::propose>;
         using approve_action = eosio::action_wrapper<"approve"_n, &multisig::approve>;
         using unapprove_action = eosio::action_wrapper<"unapprove"_n, &multisig::unapprove>;
         using cancel_action = eosio::action_wrapper<"cancel"_n, &multisig::cancel>;
         using exec_action = eosio::action_wrapper<"exec"_n, &multisig::exec>;
         using invalidate_action = eosio::action_wrapper<"invalidate"_n, &multisig::invalidate>;
         
      private:
         struct [[eosio::table]] proposal {
            name                            proposal_name;
            std::vector<char>               packed_transaction;

            uint64_t primary_key()const { return proposal_name.value; }
         };

         typedef eosio::multi_index< "proposal"_n, proposal > proposals;

         struct [[eosio::table]] old_approvals_info {
            name                            proposal_name;
            std::vector<permission_level>   requested_approvals;
            std::vector<permission_level>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef eosio::multi_index< "approvals"_n, old_approvals_info > old_approvals;

         struct approval {
            permission_level level;
            time_point       time;
         };

         struct [[eosio::table]] approvals_info {
            uint8_t                 version = 1;
            name                    proposal_name;
            //requested approval doesn't need to cointain time, but we want requested approval
            //to be of exact the same size ad provided approval, in this case approve/unapprove
            //doesn't change serialized data size. So, we use the same type.
            std::vector<approval>   requested_approvals;
            std::vector<approval>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef eosio::multi_index< "approvals2"_n, approvals_info > approvals;

         struct [[eosio::table]] invalidation {
            name         account;
            time_point   last_invalidation_time;

            uint64_t primary_key() const { return account.value; }
         };

         typedef eosio::multi_index< "invals"_n, invalidation > invalidations;
   };
   /** @}*/ // end of @defgroup eosiomsig eosio.msig
} /// namespace eosio
