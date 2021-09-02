#include <eosio/eosio.hpp>
#include <security_group_test.hpp>
#include <eosio/security_group.hpp>

using namespace eosio;

class [[eosio::contract]] security_group_test : public contract {
   public:
      using contract::contract;

      [[eosio::action]] 
      void add( name nm );
      [[eosio::action]] 
      void remove( name nm );

      [[eosio::action]]
      bool ingroup( name nm ) const;

      [[eosio::action]] 
      std::set<name> activegroup() const;

      using add_action = action_wrapper<"add"_n, &security_group_test::add>;
      using remove_action = action_wrapper<"remove"_n, &security_group_test::remove>;
      using in_active_group_action = action_wrapper<"ingroup"_n, &security_group_test::ingroup>;
      using active_group_action = action_wrapper<"activegroup"_n, &security_group_test::activegroup>;
};

[[eosio::action]] void security_group_test::add(name nm) { eosio::add_security_group_participants({nm}); }

[[eosio::action]] void security_group_test::remove(name nm) { eosio::remove_security_group_participants({nm}); }

[[eosio::action]] bool security_group_test::ingroup(name nm) const {
   return eosio::in_active_security_group({nm});
}

[[eosio::action]] std::set<name> security_group_test::activegroup() const {
   return eosio::get_active_security_group().participants;
}