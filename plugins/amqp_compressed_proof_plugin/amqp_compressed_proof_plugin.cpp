#include <eosio/amqp_compressed_proof_plugin/amqp_compressed_proof_plugin.hpp>

#include <eosio/chain/protocol_feature_manager.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/reliable_amqp_publisher/reliable_amqp_publisher.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include <fc/io/cfile.hpp>

using namespace boost::multi_index;
using namespace std::string_literals;

namespace eosio {
static appbase::abstract_plugin& _amqp_compressed_proof_plugin = app().register_plugin<amqp_compressed_proof_plugin>();

struct action_entry {
   chain::action         action;
   std::vector<char>     return_value;
   chain::action_receipt action_receipt;

   action_entry() = default;

   action_entry(const chain::action_trace& atrace) :
     action(atrace.act), return_value(atrace.return_value), action_receipt(*atrace.receipt) {}

   bool operator<(const action_entry& other) const {
      return action_receipt.global_sequence < other.action_receipt.global_sequence;
   }
};

struct combine_nodes{};
using merkle_cmd = fc::static_variant<chain::digest_type, action_entry, combine_nodes>;

struct merkle_creator_node {
   chain::checksum_type  hash;
   std::list<merkle_cmd> cmd_stream;
};

class merkle_cmd_creator {
public:
   void add_noninterested_action(const action_entry& ae) {
      curent_level.emplace_back(merkle_creator_node{chain::digest_type::hash(ae.action_receipt)});
   }

   void add_interested_action(const action_entry& ae) {
      curent_level.emplace_back(merkle_creator_node{chain::digest_type::hash(ae.action_receipt), {ae}});
   }

   std::vector<char> generate_serialized_proof(const bool arv_activated) {
      if(curent_level.empty())
         return std::vector<char>();

      while(curent_level.size() > 1) {
         if(curent_level.size() % 2)
            curent_level.emplace_back(merkle_creator_node{curent_level.back().hash});
         for(unsigned int i = 0; i < curent_level.size(); i+=2)
            curent_level[i/2] = combine(curent_level[i], curent_level[i+1]);
         curent_level.resize(curent_level.size()/2);
      }

      return fc::raw::pack((const bool)arv_activated, curent_level[0].cmd_stream);
   }

private:
   merkle_creator_node combine(merkle_creator_node& left, merkle_creator_node& right) {
      chain::digest_type::encoder digest_enc;
      fc::raw::pack(digest_enc, chain::make_canonical_left(left.hash), chain::make_canonical_right(right.hash));

      merkle_creator_node new_node = {digest_enc.result()};

      if(left.cmd_stream.empty() && right.cmd_stream.empty()) {
         //do nothing, this is an uninteresting part of the tree
      }
      else if(left.cmd_stream.empty()) {
         new_node.cmd_stream.emplace_front(left.hash);
         new_node.cmd_stream.splice(new_node.cmd_stream.end(), right.cmd_stream);
         new_node.cmd_stream.emplace_back(combine_nodes());
      }
      else if(right.cmd_stream.empty()) {
         new_node.cmd_stream.splice(new_node.cmd_stream.end(), left.cmd_stream);
         new_node.cmd_stream.emplace_back(right.hash);
         new_node.cmd_stream.emplace_back(combine_nodes());
      }
      else {
         new_node.cmd_stream.splice(new_node.cmd_stream.end(), left.cmd_stream);
         new_node.cmd_stream.splice(new_node.cmd_stream.end(), right.cmd_stream);
         new_node.cmd_stream.emplace_back(combine_nodes());
      }

      return new_node;
   }

   std::vector<merkle_creator_node>   curent_level;
};

struct compressed_proof_generator_impl {
   compressed_proof_generator_impl(chain::controller& controller, std::vector<compressed_proof_generator::result_callback_funcs>&& callbacks) :
     callbacks(callbacks), irreversible(controller.get_read_mode() == chain::db_read_mode::IRREVERSIBLE) {
      controller_connections.emplace_back(controller.irreversible_block.connect([this,&controller](const chain::block_state_ptr& bsp) {
         on_irreversible_block(bsp, controller);
      }));
      controller_connections.emplace_back(controller.applied_transaction.connect([this](std::tuple<const chain::transaction_trace_ptr&, const chain::packed_transaction_ptr&> t) {
         on_applied_transaction(std::get<0>(t));
      }));
      controller_connections.emplace_back(controller.accepted_block.connect([this,&controller](const chain::block_state_ptr& p) {
         on_accepted_block(p, controller);
      }));
      controller_connections.emplace_back(controller.block_start.connect([this](uint32_t block_num) {
         on_block_start(block_num);
      }));
   }

   void on_applied_transaction(const chain::transaction_trace_ptr& trace) {
      if(!trace->receipt)
         return;
      //only executed & delayed transaction traces would make it in to action_mroot
      if(trace->receipt->status != chain::transaction_receipt::status_enum::executed &&
         trace->receipt->status != chain::transaction_receipt::status_enum::delayed)
         return;

      if(chain::is_onblock(*trace))
         onblock_trace.emplace(trace);
      else
         cached_traces[trace->id] = trace;
   }

   void on_accepted_block(const chain::block_state_ptr& bsp, const chain::controller& controller) {
      std::set<action_entry> action_entries_this_block;

      if(onblock_trace) {
         for(const chain::action_trace& at : (*onblock_trace)->action_traces)
            action_entries_this_block.emplace(at);
      }
      for(const chain::transaction_receipt& r : bsp->block->transactions) {
         transaction_id_type id;
         if(r.trx.contains<chain::transaction_id_type>())
            id = r.trx.get<chain::transaction_id_type>();
         else
            id = r.trx.get<chain::packed_transaction>().id();
         auto it = cached_traces.find(id);
         EOS_ASSERT(it != cached_traces.end(), chain::misc_exception, "missing trace for transaction ${id}", ("id", id));
         for(const chain::action_trace& at : it->second->action_traces)
            action_entries_this_block.emplace(at);
      }
      clear_caches();

      if(irreversible)
         reversible_action_entries_index.emplace(reversible_action_entries{bsp->block_num, bsp->id, std::move(action_entries_this_block)});
      else
         run_callbacks_on_block(bsp, action_entries_this_block, controller);
   }

   void on_irreversible_block(const chain::block_state_ptr& bsp, const chain::controller& controller) {
      if(!irreversible)
         return;

      if(const auto it = reversible_action_entries_index.find(bsp->id); it != reversible_action_entries_index.end()) {
         const std::set<action_entry>& action_entries = it->action_entries;

         run_callbacks_on_block(bsp, action_entries, controller);

         reversible_action_entries_index.get<by_block_num>().erase(
                 reversible_action_entries_index.get<by_block_num>().begin(),
                 reversible_action_entries_index.get<by_block_num>().upper_bound(bsp->block_num)
         );
      }
   }

   void run_callbacks_on_block(const chain::block_state_ptr& bsp, const std::set<action_entry>& action_entries, const chain::controller& controller) {
      const chain::digest_type arv_dig = *controller.get_protocol_feature_manager().get_builtin_digest(chain::builtin_protocol_feature_t::action_return_value);
      const auto& protocol_features_this_block = bsp->activated_protocol_features->protocol_features;
      const bool action_return_value_active_this_block = protocol_features_this_block.find(arv_dig) != protocol_features_this_block.end();

      for(const auto& cb : callbacks) {
         merkle_cmd_creator cc;
         bool any_interested = false;
         for(const action_entry& ae : action_entries) {
            if(cb.first(ae.action)) {
               cc.add_interested_action(ae);
               any_interested = true;
            }
            else
               cc.add_noninterested_action(ae);
         }

         if(any_interested)
            cb.second(cc.generate_serialized_proof(action_return_value_active_this_block));
      }
   }

   void on_block_start(uint32_t block_num) {
      clear_caches();
   }

   void clear_caches() {
      cached_traces.clear();
      onblock_trace.reset();
   }

   struct reversible_action_entries {
      chain::block_num_type  block_num;
      chain::block_id_type   block_id;
      std::set<action_entry> action_entries;
   };

   struct by_block_id;
   struct by_block_num;

   typedef multi_index_container<
      reversible_action_entries,
      indexed_by<
         hashed_unique<tag<by_block_id>,   key<&reversible_action_entries::block_id>, std::hash<fc::sha256>>,
         ordered_unique<tag<by_block_num>, key<&reversible_action_entries::block_num>>
      >
   > reversible_action_entries_index_type;
   reversible_action_entries_index_type reversible_action_entries_index;

   std::map<transaction_id_type, chain::transaction_trace_ptr>    cached_traces;
   std::optional<chain::transaction_trace_ptr>                    onblock_trace;
   std::vector<compressed_proof_generator::result_callback_funcs> callbacks;

   fc::optional<boost::filesystem::path> reversible_path;

   std::list<boost::signals2::scoped_connection> controller_connections;
   bool irreversible;
};

using generator_impl = compressed_proof_generator_impl;

compressed_proof_generator::compressed_proof_generator(chain::controller& controller, std::vector<result_callback_funcs>&& callbacks) :
  my(new compressed_proof_generator_impl(controller, std::move(callbacks))) {}

compressed_proof_generator::compressed_proof_generator(chain::controller& controller, std::vector<result_callback_funcs>&& callbacks, const boost::filesystem::path& p) :
  my(new compressed_proof_generator_impl(controller, std::move(callbacks))) {
   my->reversible_path = p;

   if(boost::filesystem::exists(*my->reversible_path)) {
      try {
         fc::datastream<fc::cfile> file;
         file.set_file_path(*my->reversible_path);
         file.open("rb");
         fc::unsigned_int count;
         fc::raw::unpack(file, count);
         while(count.value--) {
            compressed_proof_generator_impl::reversible_action_entries entry;
            fc::raw::unpack(file, entry);
            my->reversible_action_entries_index.emplace(entry);
         }
      } FC_RETHROW_EXCEPTIONS(error, "Failed to load compressed merkle generator reversible state file ${f}", ("f", (fc::path)*my->reversible_path));
   }
   else {
      boost::filesystem::ofstream s(*my->reversible_path);
      EOS_ASSERT(s.good(), chain::plugin_config_exception, "Unable to create compressed merkle generator reversible state file ${f}", ("f", (fc::path)*my->reversible_path));
   }
   boost::system::error_code ec;
   boost::filesystem::remove(*my->reversible_path, ec);
}

compressed_proof_generator::~compressed_proof_generator() {
   if(my->reversible_path) {
      try {
         fc::unsigned_int num = my->reversible_action_entries_index.size();
         fc::datastream<fc::cfile> file;
         file.set_file_path(*my->reversible_path);
         file.open("wb");
         fc::raw::pack(file, num);
         for(const auto& v : my->reversible_action_entries_index)
            fc::raw::pack(file, v);
      } FC_LOG_AND_DROP();
   }
}

struct amqp_compressed_proof_plugin_impl {
   std::unique_ptr<compressed_proof_generator> proof_generator;
   std::list<reliable_amqp_publisher>          publishers;
};

amqp_compressed_proof_plugin::amqp_compressed_proof_plugin():my(new amqp_compressed_proof_plugin_impl()){}
amqp_compressed_proof_plugin::~amqp_compressed_proof_plugin() = default;

void amqp_compressed_proof_plugin::set_program_options(options_description& cli, options_description& cfg) {
   cfg.add_options()
         ("compressed-proof", bpo::value<std::vector<std::string>>()->composing(),
          "Define a filter, AMQP server, and exchange to publish compressed proofs to. May be specified multiple times. "
          "Must be in the form of:\n"
          "  <name>=<filter>=<server>=<exchange>\n"
          "Where:\n"
          "   <name>     \tis a unique name identifying this proof; used for persistent filenames\n"
          "   <filter>   \tis a comma separated list of account names to filter actions on\n"
          "   <server>   \tis the AMQP server URI\n"
          "   <exchange> \tis the AMQP exchange to publish to\n"
          )
         ;

   cli.add_options()
         ("compressed-proof-delete", bpo::bool_switch(),
          "Delete all compressed proof data files for unconfirmed messages and blocks");
}

void amqp_compressed_proof_plugin::plugin_initialize(const variables_map& options) {
   try {
      boost::filesystem::path dir = app().data_dir() / "amqp";
      boost::system::error_code ec;
      boost::filesystem::create_directories(dir, ec);

      const std::string file_prefix = "cproof";

      chain::controller& controller = app().get_plugin<chain_plugin>().chain();

      if(options.count("compressed-proof-delete") && options.at("compressed-proof-delete").as<bool>())
         for(const auto& p : boost::filesystem::directory_iterator(dir))
            if(boost::starts_with(p.path().filename().generic_string(), file_prefix))
               boost::filesystem::remove(p.path(), ec);

      const boost::filesystem::path reversible_blocks_file = dir / (file_prefix + "-reversible.bin"s);

      std::vector<compressed_proof_generator::result_callback_funcs> callbacks;

      if(options.count("compressed-proof")) {
         const std::vector<std::string>& descs = options.at("compressed-proof").as<std::vector<std::string>>();
         std::set<std::string> names_used;

         for(const std::string& desc : descs) {
            std::vector<std::string> tokens;
            boost::split(tokens, desc, boost::is_any_of("="));
            EOS_ASSERT(tokens.size() >= 4, chain::plugin_config_exception, "Did not find 4 tokens in compressed-proof option \"${o}\"", ("o", desc));

            const std::string& name = tokens[0];
            EOS_ASSERT(name.size(), chain::plugin_config_exception, "Cannot have empty name for compressed-proof \"${o}\"", ("o", desc));
            EOS_ASSERT(names_used.emplace(name).second,
                       chain::plugin_config_exception, "Name \"${n}\" used for more than one compressed-proof", ("n", name));
            std::vector<std::string> filtered_receivers;
            boost::split(filtered_receivers, tokens[1], boost::is_any_of(","));
            EOS_ASSERT(filtered_receivers.size(), chain::plugin_config_exception, "Cannot have empty filter list for compressed-proof");

            const boost::filesystem::path amqp_unconfimed_file   = dir / (file_prefix + "-unconfirmed-"s + name + ".bin"s);
            reliable_amqp_publisher& publisher = my->publishers.emplace_back(tokens[2], tokens[3], "", amqp_unconfimed_file, "eosio.node.compressed_proof_v0");

            std::set<chain::name> filter_on_names;
            for(const std::string& name_string : filtered_receivers)
               filter_on_names.emplace(name_string);

            callbacks.emplace_back(std::make_pair(
               [filter_on_names](const chain::action& act) {
                  return filter_on_names.find(act.account) != filter_on_names.end();
               },
               [&publisher](std::vector<char>&& serialized_proof) {
                  publisher.publish_message_raw(std::move(serialized_proof));
               }
            ));
         }
      }

      my->proof_generator = std::make_unique<compressed_proof_generator>(controller, std::move(callbacks), reversible_blocks_file);
   }
   FC_LOG_AND_RETHROW()
}

void amqp_compressed_proof_plugin::plugin_startup() {}

void amqp_compressed_proof_plugin::plugin_shutdown() {}

}

namespace fc {
template<typename T>
inline T& operator<<(T& ds, const std::list<eosio::merkle_cmd>& mcl) {
   fc::raw::pack(ds, unsigned_int((uint32_t)mcl.size()) );
   for(const eosio::merkle_cmd& mc : mcl)
      fc::raw::pack(ds, mc);
   return ds;
}
template<typename T>
inline T& operator<<(T& ds, const std::set<eosio::action_entry>& aes) {
   fc::raw::pack(ds, unsigned_int((uint32_t)aes.size()) );
   for(const eosio::action_entry& ae : aes)
      fc::raw::pack(ds, ae);
   return ds;
}
}

FC_REFLECT(eosio::combine_nodes, );
FC_REFLECT(eosio::action_entry, (action)(return_value)(action_receipt));
FC_REFLECT(eosio::compressed_proof_generator_impl::reversible_action_entries, (block_num)(block_id)(action_entries));