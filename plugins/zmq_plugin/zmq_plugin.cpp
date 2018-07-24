/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @author cc32d9 <cc32d9@gmail.com>
 */
#include <eosio/zmq_plugin/zmq_plugin.hpp>
#include <string>
#include <zmq.hpp>
#include <fc/io/json.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace {
  const char* SENDER_BIND = "zmq-sender-bind";
  const char* SENDER_BIND_DEFAULT = "tcp://127.0.0.1:5556";
}


namespace eosio {
  using namespace chain;
  using boost::signals2::scoped_connection;

  static appbase::abstract_plugin& _zmq_plugin = app().register_plugin<zmq_plugin>();

  struct resource_balance {
    name                       account_name;
    int64_t                    ram_quota  = 0;
    int64_t                    ram_usage = 0;
    int64_t                    net_weight = 0;
    int64_t                    cpu_weight = 0;
  };

  struct currency_balance {
    name                       account_name;
    name                       issuer;
    asset                      balance;
  };

  struct zmq_action_object {
    uint64_t                     global_action_seq;
    uint32_t                     block_num;
    chain::block_timestamp_type  block_time;
    fc::variant                  action_trace;
    vector<resource_balance>     resource_balances;
    vector<currency_balance>     currency_balances;
  };
  
  class zmq_plugin_impl {
  public:
    zmq::context_t context;
    zmq::socket_t sender_socket;
    chain_plugin*          chain_plug = nullptr;
    fc::microseconds       abi_serializer_max_time;
    std::map<name,int>     system_accounts;
    std::map<name,std::map<name,int>>  blacklist_actions;

    fc::optional<scoped_connection> applied_transaction_connection;

    zmq_plugin_impl():
      context(1),
      sender_socket(context, ZMQ_PUSH)
    {
      std::vector<name> sys_acc_names = {
        chain::config::system_account_name,
        N(eosio.msig),  N(eosio.token),  N(eosio.ram), N(eosio.ramfee),
        N(eosio.stake), N(eosio.vpay), N(eosio.bpay), N(eosio.saving)
      };

      for(name n : sys_acc_names) {
        system_accounts.emplace(std::make_pair(n, 1));
      }
      
      blacklist_actions.emplace
        (std::make_pair(chain::config::system_account_name,
                        std::map<name,int>{ std::make_pair(N(onblock), 1) }));
    }


    void on_applied_transaction( const transaction_trace_ptr& trace )
    {
      for( const auto& atrace : trace->action_traces )
        {
          on_action_trace( atrace );
        }
    }

    void on_action_trace( const action_trace& at )
    {
      // check the action against the blacklist
      auto search_acc = blacklist_actions.find(at.act.account);
      if(search_acc != blacklist_actions.end()) {
        auto search_act = search_acc->second.find(at.act.name);
        if( search_act != search_acc->second.end() ) {
          return;
        }
      }
      
      auto& chain = chain_plug->chain();

      zmq_action_object zao;
      zao.global_action_seq = at.receipt.global_sequence;
      zao.block_num = chain.pending_block_state()->block_num;
      zao.block_time = chain.pending_block_time();
      zao.action_trace = chain.to_variant_with_abi(at, abi_serializer_max_time);

      std::map<name,int> accounts;
      std::map<name,int> token_contracts;

      find_accounts_and_tokens(at, accounts, token_contracts);

      for (auto it = accounts.begin(); it != accounts.end(); ++it) {
        name account_name = it->first;
        add_account_resource( zao, account_name );
        for (auto it2 = token_contracts.begin(); it2 != token_contracts.end(); ++it2) {
          add_currency_balances( zao, account_name, it2->first );
        }
      }

      string zao_json = fc::json::to_string(zao);
      //idump((zao_json));

      int32_t msgtype = 0;
      int32_t msgopts = 0;

      zmq::message_t message(zao_json.length()+sizeof(msgtype)+sizeof(msgopts));
      unsigned char* ptr = (unsigned char*) message.data();
      memcpy(ptr, &msgtype, sizeof(msgtype));
      ptr += sizeof(msgtype);
      memcpy(ptr, &msgopts, sizeof(msgopts));
      ptr += sizeof(msgopts);
      memcpy(ptr, zao_json.c_str(), zao_json.length());
      sender_socket.send(message);
    }

    void find_accounts_and_tokens(const action_trace& at,
                                  std::map<name,int>& accounts,
                                  std::map<name,int>& token_contracts)
    {
      if( is_account_of_interest(at.act.account) ) {
        accounts.emplace(std::make_pair(at.act.account, 1));
      }

      if( at.receipt.receiver != at.act.account &&
          is_account_of_interest(at.receipt.receiver) ) {
        accounts.emplace(std::make_pair(at.receipt.receiver, 1));
      }

      if( at.act.name == N(issue) || at.act.name == N(transfer) ) {
        token_contracts.emplace(std::make_pair(at.act.account, 1));
      }

      for( const auto& iline : at.inline_traces ) {
        find_accounts_and_tokens( iline, accounts, token_contracts );
      }
    }

    bool is_account_of_interest(name account_name)
    {
      auto search = system_accounts.find(account_name);
      if(search != system_accounts.end()) {
        return false;
      }
      return true;
    }

    void add_account_resource( zmq_action_object& zao, name account_name )
    {
      resource_balance bal;
      bal.account_name = account_name;

      auto& chain = chain_plug->chain();
      const auto& rm = chain.get_resource_limits_manager();

      rm.get_account_limits( account_name, bal.ram_quota, bal.net_weight, bal.cpu_weight );
      bal.ram_usage = rm.get_account_ram_usage( account_name );
      zao.resource_balances.emplace_back(bal);
    }

    void add_currency_balances( zmq_action_object& zao,
                                name account_name, name token_code )
    {
      const auto& chain = chain_plug->chain();
      const auto& db = chain.db();

      const auto* t_id = db.find<chain::table_id_object, chain::by_code_scope_table>
        (boost::make_tuple( token_code, account_name, N(accounts) ));
      if( t_id != nullptr ) {
          const auto &idx = db.get_index<key_value_index, by_scope_primary>();
          decltype(t_id->id) next_tid(t_id->id._id + 1);
          auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
          auto upper = idx.lower_bound(boost::make_tuple(next_tid));

          for (auto obj = lower; obj != upper; ++obj) {
            if( obj->value.size() >= sizeof(asset) ) {
              asset bal;
              fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
              fc::raw::unpack(ds, bal);
              if( bal.get_symbol().valid() ) {
                zao.currency_balances.emplace_back(currency_balance{account_name, token_code, bal});
              }
            }
          }
      }
    }
  };


  zmq_plugin::zmq_plugin():my(new zmq_plugin_impl()){}
  zmq_plugin::~zmq_plugin(){}

  void zmq_plugin::set_program_options(options_description&, options_description& cfg)
  {
    cfg.add_options()
      (SENDER_BIND, bpo::value<string>()->default_value(SENDER_BIND_DEFAULT),
       "ZMQ Sender Socket binding")
      ;
  }

  void zmq_plugin::plugin_initialize(const variables_map& options)
  {
    string bind_str = options.at(SENDER_BIND).as<string>();
    if (bind_str.empty())
      {
        wlog("zmq-sender-bind not specified => eosio::zmq_plugin disabled.");
        return;
      }
    ilog("Binding to ${u}", ("u", bind_str));
    my->sender_socket.bind(bind_str);

    my->chain_plug = app().find_plugin<chain_plugin>();
    my->abi_serializer_max_time = my->chain_plug->get_abi_serializer_max_time();
    
    auto& chain = my->chain_plug->chain();
    my->applied_transaction_connection.emplace(chain.applied_transaction.connect( [&]( const transaction_trace_ptr& p ){
          my->on_applied_transaction(p);
        }));
  }

  void zmq_plugin::plugin_startup() {

  }

  void zmq_plugin::plugin_shutdown() {
  }

}

FC_REFLECT( eosio::resource_balance,
            (account_name)(ram_quota)(ram_usage)(net_weight)(cpu_weight) )

FC_REFLECT( eosio::currency_balance,
            (account_name)(issuer)(balance))

FC_REFLECT( eosio::zmq_action_object,
            (global_action_seq)(block_num)(block_time)(action_trace)
            (resource_balances)(currency_balances) )

