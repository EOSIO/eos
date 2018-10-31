/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <zmq.hpp>
#include <eosio/statetrack_plugin/statetrack_plugin.hpp>

namespace {
  const char* SENDER_BIND = "st-zmq-sender-bind";
  const char* SENDER_BIND_DEFAULT = "tcp://127.0.0.1:3000";
}

namespace eosio {

static appbase::abstract_plugin& _statetrack_plugin = app().register_plugin<statetrack_plugin>();
typedef typename chainbase::get_index_type<chain::key_value_object>::type kv_index_type;
  
struct filter_entry {
    name code;
    name scope;
    name table;

    std::tuple<name, name, name> key() const {
       return std::make_tuple(code, scope, table);
    }

    friend bool operator<( const filter_entry& a, const filter_entry& b ) {
       return a.key() < b.key();
    }
 };

class statetrack_plugin_impl {
   public:
        void send_zmq_msg(const string content);
        bool filter(const chain::table_id_object& tio);

        const chain::table_id_object& get_kvo_tio(const chainbase::database& db, const chain::key_value_object& kvo);
        const chain::account_object& get_tio_co(const chainbase::database& db, const chain::table_id_object& tio);
        const abi_def& get_co_abi(const chain::account_object& co );
        fc::variant get_kvo_row(const chainbase::database& db, const chain::table_id_object& tio, const chain::key_value_object& kvo, bool json = true);
        db_op get_db_op(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type, bool json = true);
        db_rev get_db_rev(const int64_t revision, op_type_enum op_type);
  
        void on_applied_op(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type);
        void on_applied_rev(const int64_t revision, op_type_enum op_type);

        static void copy_inline_row(const chain::key_value_object& obj, vector<char>& data) {
            data.resize( obj.value.size() );
            memcpy( data.data(), obj.value.data(), obj.value.size() );
        }

        static fc::string scope_sym_to_string(uint64_t sym_code) {
            fc::string scope = sym_code.to_string();
            if(scope.length() > 0 && scope[0] == '.') {
                vector<char> v;
                for( int i = 0; i < 7; ++i ) {
                    char c = (char)(sym_code & 0xff);
                    if( !c ) break;
                    v.emplace_back(c);
                    sym_code >>= 8;
                }
                return fc::string(v.begin(),v.end());
            }
            return scope;
        }

        fc::string socket_bind_str;
        zmq::context_t* context;
        zmq::socket_t* sender_socket;
  
        std::vector<filter_entry> filter_on;
        std::vector<filter_entry> filter_out;
        fc::microseconds abi_serializer_max_time;
    private:
        bool shorten_abi_errors = true;
};

// statetrack plugin implementation

void statetrack_plugin_impl::send_zmq_msg(const fc::string content) {
  zmq::message_t message(content.length());
  memcpy(message.data(), content.c_str(), content.length());
  sender_socket->send(message);
}
  
bool statetrack_plugin_impl::filter(const chain::table_id_object& tio) {
  auto code = tio.code;
  auto scope = scope_sym_to_string(tio.scope);
  auto table = tio.table;
  
  if(filter_on.size() > 0) {
    bool pass_on = false;
    if (filter_on.find({ code, 0, 0 }) != filter_on.end()) {
      pass_on = true;
    }
    if (filter_on.find({ code, scope, 0 }) != filter_on.end()) {
      pass_on = true;
    }
    if (filter_on.find({ code, scope, table }) != filter_on.end()) {
      pass_on = true;
    }
    if (!pass_on) {  return false;  }
  }

  if(filter_out.size() > 0) {
    if (filter_out.find({ code, 0, 0 }) != filter_out.end()) {
      return false;
    }
    if (filter_out.find({ code, scope, 0 }) != filter_out.end()) {
      return false;
    }
    if (filter_out.find({ code, scope, table }) != filter_out.end()) {
      return false;
    }
  }

  return true;
}
   
const chain::table_id_object& statetrack_plugin_impl::get_kvo_tio(const chainbase::database& db, const chain::key_value_object& kvo) {
    const chain::table_id_object *tio = db.find<chain::table_id_object>(kvo.t_id);
    EOS_ASSERT(tio != nullptr, chain::contract_table_query_exception, "Fail to retrieve table for ${t_id}", ("t_id", kvo.t_id) );
    return *tio;
}

const chain::account_object& statetrack_plugin_impl::get_tio_co(const chainbase::database& db, const chain::table_id_object& tio) {
    const chain::account_object *co = db.find<chain::account_object, chain::by_name>(tio.code);
    EOS_ASSERT(co != nullptr, chain::account_query_exception, "Fail to retrieve account for ${code}", ("code", tio.code) );
    return *co;
}

const abi_def& statetrack_plugin_impl::get_co_abi( const chain::account_object& co ) {   
   abi_def* abi = new abi_def();
   abi_serializer::to_abi(co.abi, *abi);
   return *abi;
}

fc::variant statetrack_plugin_impl::get_kvo_row(const chainbase::database& db, const chain::table_id_object& tio, const chain::key_value_object& kvo, bool json) {
    vector<char> data;
    copy_inline_row(kvo, data);

    if (json) {
        const chain::account_object& co = get_tio_co(db, tio);
        const abi_def& abi = get_co_abi(co);

        abi_serializer abis;
        abis.set_abi(abi, abi_serializer_max_time);

        return abis.binary_to_variant( abis.get_table_type(tio.table), data, abi_serializer_max_time, shorten_abi_errors );
    } else {
        return fc::variant(data);
    }
}

db_op statetrack_plugin_impl::get_db_op(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type, bool json) {
    db_op op;

    const chain::table_id_object& tio = get_kvo_tio(db, kvo);

    op.id = kvo.id;
    op.op_type = op_type;
    op.code = tio.code;
    op.scope = scope_sym_to_string(tio.scope);
    op.table = tio.table;

    if(op_type == op_type_enum::CREATE || 
       op_type == op_type_enum::MODIFY) {
        op.value = get_kvo_row(db, tio, kvo, json);
    }

    return op;
}

db_rev statetrack_plugin_impl::get_db_rev(const int64_t revision, op_type_enum op_type) {
    db_rev rev;
    rev.op_type = op_type;
    rev.revision = revision;
    return rev;
}
  
void statetrack_plugin_impl::on_applied_op(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type) {
  if(filter(get_kvo_tio(db, kvo))) {
    fc::string data = fc::json::to_string(get_db_op(db, kvo, op_type));
    ilog("STATETRACK key_value_object {op_type}: ${data}", ("op_type", op_type), ("data", data));
    send_zmq_msg(data);
  }
}
  
void statetrack_plugin_impl::on_applied_rev(const int64_t revision, op_type_enum op_type) {
  fc::string data = fc::json::to_string(get_db_rev(revision, op_type));
  ilog("STATETRACK {op_type}: ${data}", ("op_type", op_type), ("data", data));
  send_zmq_msg(data);
}

// Plugin implementation

statetrack_plugin::statetrack_plugin():my(new statetrack_plugin_impl()){}

void statetrack_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
      (SENDER_BIND, bpo::value<string>()->default_value(SENDER_BIND_DEFAULT),
       "ZMQ Sender Socket binding")
      ;
   cfg.add_options()
      ("st-filter-on,f", bpo::value<vector<string>>()->composing(),
       "Track tables which match code:scope:table.")
      ;
   cfg.add_options()
      ("st-filter-out,F", bpo::value<vector<string>>()->composing(),
       "Do not track tables which match code:scope:table.")
      ;
}

void statetrack_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing statetrack plugin");

   try {
     if( options.count( "st-filter-on" )) {
        auto fo = options.at( "st-filter-on" ).as<vector<string>>();
        for( auto& s : fo ) {
           std::vector<std::string> v;
           boost::split( v, s, boost::is_any_of( ":" ));
           EOS_ASSERT( v.size() == 3, fc::invalid_arg_exception, "Invalid value ${s} for --filter-on", ("s", s));
           filter_entry fe{v[0], v[1], v[2]};
           my->filter_on.insert( fe );
        }
     }
     
     if( options.count( "st-filter-out" )) {
        auto fo = options.at( "st-filter-out" ).as<vector<string>>();
        for( auto& s : fo ) {
           std::vector<std::string> v;
           boost::split( v, s, boost::is_any_of( ":" ));
           EOS_ASSERT( v.size() == 3, fc::invalid_arg_exception, "Invalid value ${s} for --filter-out", ("s", s));
           filter_entry fe{v[0], v[1], v[2]};
           my->filter_out.insert( fe );
        }
     }

      my->socket_bind_str = options.at(SENDER_BIND).as<string>();
      if (my->socket_bind_str.empty()) {
         wlog("zmq-sender-bind not specified => eosio::statetrack_plugin disabled.");
         return;
      }
     
      my->context = new zmq::context_t(1);
      my->sender_socket = new zmq::socket_t(*my->context, ZMQ_PUSH);
      
      ilog("Bind to ZMQ PUSH socket ${u}", ("u", my->socket_bind_str));
      my->sender_socket->connect(my->socket_bind_str);

      ilog("Bind to ZMQ PUSH socket successful");
      
      chain_plugin& chain_plug = app().get_plugin<chain_plugin>();
      const chainbase::database& db = chain_plug.chain().db();

      my->abi_serializer_max_time = chain_plug.get_abi_serializer_max_time();
      
      ilog("Binding database events");
      
      auto& kv_index = db.get_index<kv_index_type>();
    
      kv_index.applied_emplace = [&](const chain::key_value_object& kvo) {
          my->on_applied_op(db, kvo, op_type_enum::CREATE);
      };
  
      kv_index.applied_modify = [&](const chain::key_value_object& kvo) {
          my->on_applied_op(db, kvo, op_type_enum::MODIFY);
      };
  
      kv_index.applied_remove = [&](const chain::key_value_object& kvo) {
          my->on_applied_op(db, kvo, op_type_enum::REMOVE);
      };
     
      kv_index.applied_undo = [&](const int64_t revision) {
          my->on_applied_rev(revision, op_type_enum::UNDO);
      };
     
      /*kv_index.applied_squash = [&](const int64_t revision) {
          my->on_applied_rev(revision, op_type_enum::SQUASH);
      };*/
     
      kv_index.applied_commit = [&](const int64_t revision) {
          my->on_applied_rev(revision, op_type_enum::COMMIT);
      };
   }
   FC_LOG_AND_RETHROW()
}

void statetrack_plugin::plugin_startup() {

}

void statetrack_plugin::plugin_shutdown() {
   ilog("statetrack plugin shutdown");
   if( ! my->socket_bind_str.empty() ) {
      my->sender_socket->disconnect(my->socket_bind_str);
      my->sender_socket->close();
      delete my->sender_socket;
      delete my->context;
   }
}

}
