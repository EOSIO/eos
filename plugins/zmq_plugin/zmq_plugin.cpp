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

  struct zmq_action_object {
    const action_trace   atrace;
    uint32_t             block_num;
    block_timestamp_type block_time;

    zmq_action_object(const action_trace& at) : atrace(at) {};
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
      
      zmq_action_object zao(at);
      zao.block_num = chain.pending_block_state()->block_num;
      zao.block_time = chain.pending_block_time();
      
      string zao_json = fc::json::to_string(zao);
      idump((zao_json));

      int64_t msgtype = 0;
      
      zmq::message_t message(zao_json.length()+sizeof(msgtype));
      memcpy(message.data(), &msgtype, sizeof(msgtype));
      memcpy((unsigned char*)message.data()+sizeof(msgtype), zao_json.c_str(), zao_json.length());
      sender_socket.send(message);
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


FC_REFLECT( eosio::zmq_action_object, 
            (atrace)(block_num)(block_time) )

