#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <vector>

#define SUCCESS_CODE 0
#define NOT_ENOUGH_ALLOWED  "6000017 NOT_ENOUGH_ALLOWED"


namespace oct {
   using std::string;
   using std::array;

    using namespace eosio;
    using boost::container::flat_map;

    struct approvetoPair
    {
        account_name to;
        uint64_t value;
        approvetoPair() {}
        EOSLIB_SERIALIZE(approvetoPair, (to)(value))
    };
    /*
    @abi table approves
    eosiocpp flat_map and map not support now
    */
    struct approveto {
       uint64_t symbol_name;
       std::vector<approvetoPair> approved;
       uint64_t primary_key()const { return symbol_name; }
       EOSLIB_SERIALIZE( approveto, (symbol_name)(approved))
    };

    /*
    @abi action approve
    */
    struct approveact {
        account_name owner;
        account_name spender;
        asset quantity;
        EOSLIB_SERIALIZE(approveact, (owner)(spender)(quantity))
    };

    /*
    @abi action transferfrom
    */
    struct transferfromact {
        account_name from;
        account_name to;
        asset quantity;
        EOSLIB_SERIALIZE(transferfromact, (from)(to)(quantity))
    };

    /*
     *@abi action balanceof
    */
    struct balanceOfAct{
        account_name owner;
        std::string  symbol;
        EOSLIB_SERIALIZE(balanceOfAct, (owner)(symbol))
    };


    /*
     *@abi action allowanceof
    */
    struct allowanceOfAct{
        account_name owner;
        account_name spender;
        std::string  symbol;
        EOSLIB_SERIALIZE(allowanceOfAct, (owner)(spender)(symbol))
    };
   /**
    *  This contract enables the creation, issuance, and transfering of many different tokens.
    *
    */


   class currency {
      public:
         currency( account_name contract )
         :_contract(contract)
         { }

         struct create {
            account_name           issuer;
            asset                  maximum_supply;
            uint8_t                issuer_can_freeze     = true;
            uint8_t                issuer_can_recall     = true;
            uint8_t                issuer_can_whitelist  = true;

            /*(issuer_agreement_hash)*/
            EOSLIB_SERIALIZE( create, (issuer)(maximum_supply)(issuer_can_freeze)(issuer_can_recall)(issuer_can_whitelist) )
         };

         struct transfer
         {
            account_name from;
            account_name to;
            asset        quantity;
            string       memo;

            EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
         };

         struct issue {
            account_name to;
            asset        quantity;
            string       memo;

            EOSLIB_SERIALIZE( issue, (to)(quantity)(memo) )
         };

         struct fee_schedule {
            uint64_t primary_key()const { return 0; }

            array<extended_asset,7> fee_per_length;
            EOSLIB_SERIALIZE( fee_schedule, (fee_per_length) )
         };

         struct account {
            asset    balance;
            bool     frozen    = false;
            bool     whitelist = true;

            uint64_t primary_key()const { return balance.symbol; }

            EOSLIB_SERIALIZE( account, (balance)(frozen)(whitelist) )
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;
            bool           can_freeze         = true;
            bool           can_recall         = true;
            bool           can_whitelist      = true;
            bool           is_frozen          = false;
            bool           enforce_whitelist  = false;

            uint64_t primary_key()const { return supply.symbol.name(); }

            EOSLIB_SERIALIZE( currency_stats, (supply)(max_supply)(issuer)(can_freeze)(can_recall)(can_whitelist)(is_frozen)(enforce_whitelist) )
         };

         void approve(const approveact & approveobj);

         /// @notice send `_value` token to `_to` from `_from` on the condition it is approved by `_from`
         /// @param from The account of the sender
         /// @param to The account of the recipient
         /// @param quantity The amount of token to be transferred
         void transferFrom(const transferfromact& tfa);

         /// @param owner The account from which the balance will be retrieved
         /// @param symbol
         void balanceOf(const balanceOfAct & boa);

         void allowanceOf(const allowanceOfAct & aof);

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stats> stats;
         typedef eosio::multi_index<N(approves), approveto> approves;


         asset get_balance( account_name owner, symbol_name symbol )const {
            accounts t( _contract, owner );
            return t.get(symbol).balance;
         }

         asset get_supply( symbol_name symbol )const {
            accounts t( _contract, symbol );
            return t.get(symbol).balance;
         }

         static void inline_transfer( account_name from, account_name to, extended_asset amount, string memo = string(), permission_name perm = N(active) ) {
            action act( permission_level( from, perm ), amount.contract, N(transfer), transfer{from,to,amount,memo} );
            act.send();
         }

         void inline_transfer( account_name from, account_name to, asset amount, string memo = string(), permission_name perm = N(active) ) {
            action act( permission_level( from, perm ), _contract, N(transfer), transfer{from,to,amount,memo} );
            act.send();
         }


         bool apply( account_name contract, action_name act );

          /**
           * This is factored out so it can be used as a building block
           */
          void create_currency( const create& c );

          void issue_currency( const issue& i );

          void on( const create& c );

          void on( const transfer& t );

          void on( const issue& i );


      private:
          void sub_balance( account_name owner, asset value, const currency_stats& st );

          void add_balance( account_name owner, asset value, const currency_stats& st, account_name ram_payer );

      private:
         account_name _contract;
   };

}
