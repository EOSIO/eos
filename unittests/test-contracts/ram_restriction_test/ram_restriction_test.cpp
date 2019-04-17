/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>

class [[eosio::contract]] ram_restriction_test : public eosio::contract {

public:
   struct [[eosio::table]] data {
      uint64_t           key;
      std::vector<char>  value;

      uint64_t primary_key() const { return key; }
   };

   typedef eosio::multi_index<"tablea"_n, data> tablea;
   typedef eosio::multi_index<"tableb"_n, data> tableb;

public:
   using eosio::contract::contract;

   template <typename Table>
   void setdata_(int len, eosio::name payer) {
      Table ta(_self, 0);
      std::vector<char> data;
      data.resize(len, 0);
      auto it = ta.find(0);
      if (it == ta.end()) {
         if (len) {
            ta.emplace(payer, [&](auto &v) {
               v.key = 0;
               v.value = data;
            });
         }
      } else {
         if (len) {
            ta.modify(it, payer, [&](auto &v) {
               v.key = 0;
               v.value = data;
            });
         } else {
            ta.erase(it);
         }
      }
   }

   [[eosio::action]]
   void setdata(int len1, int len2, eosio::name payer ) {
      setdata_<tablea>(len1, payer);
      setdata_<tableb>(len2, payer);
   }

   [[eosio::action]]
   void notifysetdat(eosio::name acctonotify, int len1, int len2, eosio::name payer) {
      require_recipient(acctonotify);
   }

   [[eosio::on_notify("testacc::notifysetdat")]]
   void on_notify_setdata(eosio::name acctonotify, int len1, int len2, eosio::name payer) {
      setdata(len1, len2, payer);
   }
};
