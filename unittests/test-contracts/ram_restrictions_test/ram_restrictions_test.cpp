#include "ram_restrictions_test.hpp"
#include <eosio/transaction.hpp>

using namespace eosio;

template <typename Table>
void _setdata(name self, int len, name payer) {
   Table ta(self, 0);
   std::vector<char> data;
   data.resize(len, 0);
   auto it = ta.find(0);
   if (it == ta.end()) {
     ta.emplace(payer, [&](auto &v) {
         v.key = 0;
         v.value = data;
     });
   } else {
     ta.modify(it, payer, [&](auto &v) {
         v.key = 0;
         v.value = data;
     });
   }
}

void ram_restrictions_test::noop( ) {
}

void ram_restrictions_test::setdata( uint32_t len1, uint32_t len2, name payer ) {
   _setdata<tablea>(get_self(), len1, payer);
   _setdata<tableb>(get_self(), len2, payer);
}

void ram_restrictions_test::notifysetdat( name acctonotify, uint32_t len1, uint32_t len2, name payer ) {
   require_recipient(acctonotify);
}

void ram_restrictions_test::on_notify_setdata( name acctonotify, uint32_t len1, uint32_t len2, name payer) {
   setdata(len1, len2, payer);
}

void ram_restrictions_test::senddefer( uint64_t senderid, name payer ) {
   transaction trx;
   trx.actions.emplace_back(
      std::vector<eosio::permission_level>{{_self, "active"_n}},
      get_self(),
      "noop"_n,
      std::make_tuple()
   );
   trx.send( senderid, payer );
}

void ram_restrictions_test::notifydefer( name acctonotify, uint64_t senderid, name payer ) {
   require_recipient(acctonotify);
}

void ram_restrictions_test::on_notifydefer( name acctonotify, uint64_t senderid, name payer ) {
   senddefer(senderid, payer);
}
