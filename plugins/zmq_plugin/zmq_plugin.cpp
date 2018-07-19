/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
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
    
    fc::optional<scoped_connection> applied_transaction_connection;
    
    zmq_plugin_impl():
      context(1),
      sender_socket(context, ZMQ_PUSH)
    {
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
      auto& chain = chain_plug->chain();
      
      zmq_action_object zao;
      zao.global_action_seq = at.receipt.global_sequence;
      zao.block_num = chain.pending_block_state()->block_num;
      zao.block_time = chain.pending_block_time();
      zao.action_trace = chain.to_variant_with_abi(at);

      add_acc_resources_and_systoken_bal(zao, at.act.account );
      if( at.receipt.receiver != at.act.account ) {
        add_acc_resources_and_systoken_bal(zao, at.receipt.receiver );
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

    void add_acc_resources_and_systoken_bal(zmq_action_object& zao, name account_name )
    {
      if( account_name == chain::config::system_account_name ) {
        return;
      }
      
      if( account_name == N(eosio.token) ) {
        return;
      }

      add_account_resource( zao, account_name );
      add_currency_balances( zao, account_name, N(eosio.token) );
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

    void add_currency_balances( zmq_action_object& zao, name account_name, name token_code )
    {
      auto& chain = chain_plug->chain();
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
                zao.currency_balances.emplace_back(currency_balance{account_name, bal});
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
            (account_name)(balance))
            
FC_REFLECT( eosio::zmq_action_object, 
            (global_action_seq)(block_num)(block_time)(action_trace)
            (resource_balances)(currency_balances) )

