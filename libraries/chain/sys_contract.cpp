#include <eos/chain/database.hpp>
#include <eos/chain/sys_contract.hpp>
#include <eos/chain/account_object.hpp>

namespace eos { namespace chain {

void database::init_sys_contract() {
   wlog( "init sys contract" );

   set_validate_handler( "sys", "sys", "Transfer", [&]( message_validate_context& context ) {
       auto transfer = context.msg.as<Transfer>();
       FC_ASSERT( context.msg.has_notify( transfer.to ), "Must notify recipient of transfer" );
   });

   set_precondition_validate_handler( "sys", "sys", "Transfer", [&]( precondition_validate_context& context ) {
       auto transfer = context.msg.as<Transfer>();
       const auto& from = get_account( context.msg.sender );
       FC_ASSERT( from.balance > transfer.amount, "Insufficient Funds", 
                  ("from.balance",from.balance)("transfer.amount",transfer.amount) );
   });

   set_apply_handler( "sys", "sys", "Transfer", [&]( apply_context& context ) {
       auto transfer = context.msg.as<Transfer>();
       const auto& from = get_account( context.msg.sender );
       const auto& to   = get_account( transfer.to   );
       modify( from, [&]( account_object& a ) {
          a.balance -= transfer.amount;
       });
       modify( to, [&]( account_object& a ) {
          a.balance += transfer.amount;
       });
   });
}

} } // namespace eos::chain
