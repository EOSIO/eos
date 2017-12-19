/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */


namespace native {
  /**
    @defgroup eoscontract EOS Contract
    @brief Documents the interface to the EOS currency contract
    @ingroup contracts
    @{
  */

   /**
    *  @ingroup contracts
    *  @brief Defines the base class for all contracts
    */
   struct contract {

      /**
       *  @brief updates the code that will be executed for this contract
       *
       *  <h3> Required Authority </h3>
       *
       *  Requires authority of *this* contract.
       *
       *  <h3> Required Scope </h3>
       *
       *  Requires scope of *this* contract.
       *
       *  @note the change in code does not take effect until the start of the next block
       */
      void setcode( Bytes code,
                    Abi   abi,
                    uint8_t vm = 0,
                    uint8_t vm_version = 0 ) final;

      /**
       * @brief updates the authority required for a named permission
       *
       *  <h3> Required Authority </h3>
       *
       *  Requires authority of *this* contract.
       *
       *  <h3> Required Scope </h3>
       *
       *  Requires scope of *this* contract.
       */
      void setauth( Name permission, ///< the name for the permission being set
                    Name parent, ///< the parent permission to this permission
                    Authority auth ///< the set of keys/accounts and threshold  );
                   ) final;


      /**
       *  @brief set the local named permission required for `this` account/contract to
       *     call `con::act(...)`
       *
       *  <h3> Required Authority </h3>
       *
       *  Requires authority of *this* contract.
       *
       *  <h3> Required Scope </h3>
       *
       *  Requires scope of *this* contract.
       *
       *  @pre myperm must be defined by prior call to @ref setauth
       *
       *  @param con - the name of the contract this permission applies
       *  @param act - the name of the action on @ref con this permission applies to
       *  @param myperm - the name of a permission set by @ref setauth on `this` contract
       */
      void setperm( Name con, Name act, Name myperm );
   };

   /**
    *   @class eos
    *   @brief A *native* currency contract implemented with account named `eos`
    *   @ingroup contracts
    *
    *   @details The EOS contract is a *native* currency contract implemented with account named `eos`. This contract enables
    *   users to transfer EOS tokens to each other. This contract is designed to work the @ref stakedcontract and
    *   @ref systemcontract when creating new accounts, claiming staked EOS.
    */
   struct eos : public contract {

      /**
        @brief This action will transfer funds from one account to another.

        @pre `from`'s balance must be greaterthan or equal to `amount` transferred.
        @pre The amount transferred must be greater than 0
        @pre `to` and `from` may not be the same account.

      <h3> Required Authority </h3>

      This action requires the authority of the `from` account.

      <h3>Required Scope </h3>

      This action requires access to `from` and `to` account scopes. It does not require
      access to the `eos` scope which means that multiple transfers can execute in parallel
      as long as they don't have any overlapping scopes.

      <h3> Required Recipients </h3>

      This message requires that the accounts `from` and `to` are listed in the required recipients. This ensures
      other contracts are notified anytime EOS tokens are transferred.

      */
      void transfer (
         account_name from,  ///< account from which EOS will be withdrawn
         account_name to,    ///< account to receive EOS, may not be same as `from`
         uint64_t     amount ///< must be greater than 0 and less or equal to `from`'s balance
      );

 }; /// class EOS
 /// @}
}
