#include "restrict_action_test.hpp"
#include <eosio/transaction.hpp>

using namespace eosio;

void restrict_action_test::noop( ) {

}

void restrict_action_test::sendinline( name authorizer ) {
   action(
      permission_level{authorizer,"active"_n},
      get_self(),
      "noop"_n,
      std::make_tuple()
   ).send();
}

void restrict_action_test::senddefer( name authorizer, uint32_t senderid ) {
   transaction trx;
   trx.actions.emplace_back(
      permission_level{authorizer,"active"_n},
      get_self(),
      "noop"_n,
      std::make_tuple()
   );
   trx.send(senderid, get_self());
}

void restrict_action_test::notifyinline( name acctonotify, name authorizer ) {
   require_recipient(acctonotify);
}

void restrict_action_test::notifydefer( name acctonotify, name authorizer, uint32_t senderid ) {
   require_recipient(acctonotify);
}

void restrict_action_test::on_notify_inline( name acctonotify, name authorizer ) {
   sendinline(authorizer);
}

void restrict_action_test::on_notify_defer( name acctonotify, name authorizer, uint32_t senderid ) {
   senddefer(authorizer, senderid);
}
