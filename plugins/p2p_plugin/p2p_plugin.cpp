/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <eos/p2p_plugin/p2p_plugin.hpp>

#include <eos/net/node.hpp>
#include <eos/net/exceptions.hpp>
#include <eos/chain/exceptions.hpp>

#include <fc/network/ip.hpp>

#include <boost/range/algorithm/reverse.hpp>

using std::string;
using std::vector;

namespace eos {

class p2p_plugin_impl {
public:
   fc::optional<fc::ip::endpoint> listen_endpoint;
   vector<fc::ip::endpoint> seeds;
   string user_agent;

   std::unique_ptr<eos::net::node> node;

   class node_delegate : public eos::net::node_delegate {
   public:
      node_delegate(chain_plugin& chain)
         : chain(chain) {}
      virtual ~node_delegate() {}

      // node_delegate interface
      virtual bool has_item(const net::item_id& id) override;
      virtual bool handle_block(const net::block_message& blk_msg, bool sync_mode, std::vector<fc::uint160_t>&) override;
      virtual void handle_transaction(const net::trx_message& trx_msg) override;
      virtual void handle_message(const net::message& message_to_process) override;
      virtual std::vector<net::item_hash_t> get_block_ids(const std::vector<net::item_hash_t>& blockchain_synopsis, uint32_t& remaining_item_count, uint32_t limit) override;
      virtual net::message get_item(const net::item_id& id) override;
      virtual chain::chain_id_type get_chain_id() const override;
      virtual std::vector<net::item_hash_t> get_blockchain_synopsis(const net::item_hash_t& reference_point, uint32_t number_of_blocks_after_reference_point) override;
      virtual void sync_status(uint32_t item_type, uint32_t item_count) override;
      virtual void connection_count_changed(uint32_t c) override;
      virtual uint32_t get_block_number(const net::item_hash_t& block_id) override;
      virtual fc::time_point_sec get_block_time(const net::item_hash_t& block_id) override;
      virtual net::item_hash_t get_head_block_id() const override;
      virtual uint32_t estimate_last_known_fork_from_git_revision_timestamp(uint32_t) const override;
      virtual void error_encountered(const string& message, const fc::oexception& error) override;
      virtual uint8_t get_current_block_interval_in_seconds() const override;

      chain_plugin& chain;
   };
};

p2p_plugin::p2p_plugin()
   : my(new p2p_plugin_impl){}

p2p_plugin::~p2p_plugin() {}

void p2p_plugin::set_program_options(bpo::options_description&, bpo::options_description& config_file_options) {
   config_file_options.add_options()
         ("listen-endpoint", bpo::value<string>()->implicit_value("127.0.0.1:9876"),
          "The local IP address and port to listen for incoming connections.")
         ("remote-endpoint", bpo::value<vector<string>>()->composing(),
          "The IP address and port of a remote peer to sync with.")
         ("user-agent", bpo::value<string>()->default_value("Eos Reference Implementation"),
          "User agent to advertise to peers")
         ;
}

void p2p_plugin::plugin_initialize(const boost::program_options::variables_map& options) {
   ilog("Initialize P2P Plugin");
   if (options.count("listen-endpoint"))
      my->listen_endpoint = fc::ip::endpoint::from_string(options.at("listen-endpoint").as<string>());
   if (options.count("user-agent"))
      my->user_agent = options.at("user-agent").as<string>();

   if (options.count("remote-endpoint")) {
      auto seeds = options.at("remote-endpoint").as<vector<string>>();
      my->seeds.resize(seeds.size());
      std::transform(seeds.begin(), seeds.end(), my->seeds.begin(), &fc::ip::endpoint::from_string);
   }
}

void p2p_plugin::plugin_startup() {
   auto& chain = app().get_plugin<chain_plugin>();
   my->node.reset(new eos::net::node(my->user_agent));
   my->node->load_configuration(app().data_dir() / "p2p");
   my->node->set_node_delegate(new p2p_plugin_impl::node_delegate(chain));

   if (my->listen_endpoint) {
      ilog("Configuring P2P to listen at ${ep}", ("ep", my->listen_endpoint));
      my->node->listen_on_endpoint(*my->listen_endpoint, true);
   }

   for (const auto& seed : my->seeds) {
      ilog("P2P adding seed node ${s}", ("s", seed));
      my->node->add_node(seed);
      my->node->connect_to_endpoint(seed);
   }

   my->node->listen_to_p2p_network();
   my->node->connect_to_p2p_network();
   my->node->sync_from(net::item_id(net::block_message_type, chain.db().head_block_id()), {});
   ilog("P2P node listening at ${ep}", ("ep", my->node->get_actual_listening_endpoint()));
}

void p2p_plugin::plugin_shutdown() {
   ilog("Shutting down P2P Plugin");
   my->node->close();
   my->node.reset();
}

void p2p_plugin::broadcast_block(const chain::signed_block& block) {
   ilog("Broadcasting block #${n}", ("n", block.block_num()));
   my->node->broadcast(eos::net::block_message(block));
}

////////////////////////////// Begin node_delegate Implementation //////////////////////////////
bool p2p_plugin_impl::node_delegate::has_item(const net::item_id& id) {
   switch (id.item_type) {
   case net::block_message_type:
      return chain.db().is_known_block(id.item_hash);
   case net::trx_message_type:
      return chain.db().is_known_transaction(id.item_hash);
   default:
      elog("P2P asked us if we have an item, but it's of unknown type: ${item}", ("item", id));
      // Whatever it is, I'm assuming we don't want it.
      return true;
   }
}

bool p2p_plugin_impl::node_delegate::handle_block(const net::block_message& blk_msg, bool sync_mode,
                                                  std::vector<fc::uint160_t>&) {
   try {
      if (!sync_mode) ilog("Received block ${num} from peer", ("num", blk_msg.block.block_num()));
      return chain.accept_block(blk_msg.block, sync_mode);
   } catch (eos::chain::unlinkable_block_exception& e) {
      FC_THROW_EXCEPTION(net::unlinkable_block_exception, "Unlinkable block: ${e}", ("e", e.to_detail_string()));
   }
}

void p2p_plugin_impl::node_delegate::handle_transaction(const net::trx_message& trx_msg) {
   chain.accept_transaction(trx_msg.trx);
}

void p2p_plugin_impl::node_delegate::handle_message(const net::message& message_to_process) {
   FC_THROW("Unknown message type ${type}", ("type", message_to_process.msg_type));
}

vector<net::item_hash_t> p2p_plugin_impl::node_delegate::get_block_ids(const std::vector<net::item_hash_t>& blockchain_synopsis,
                                                                            uint32_t& remaining_item_count, uint32_t limit)
{ try {
   if (chain.db().head_block_num() == 0) {
      remaining_item_count = 0;
      return {};
   }

   auto itr = std::find_if(blockchain_synopsis.rbegin(), blockchain_synopsis.rend(),
                           [this](const net::item_hash_t& h) { return chain.block_is_on_preferred_chain(h); });

   EOS_ASSERT(blockchain_synopsis.empty() || itr != blockchain_synopsis.rend(), net::peer_is_on_an_unreachable_fork,
              "Unable to find any recognizeable block in peer's blockchain synopsis.");

   unsigned last_known_block_num = (itr != blockchain_synopsis.rend())? chain::block_header::num_from_id(*itr) : 0;

   auto head_block_num = chain.db().head_block_num();
   auto last_block_num_to_return = std::min(head_block_num, last_known_block_num + limit - 1);
   remaining_item_count = head_block_num - last_block_num_to_return;

   vector<net::item_hash_t> ids;
   ids.reserve(last_block_num_to_return - last_known_block_num);
   for (unsigned num = last_known_block_num; num <= last_block_num_to_return; ++num)
      if (BOOST_LIKELY(num > 0))
         ids.emplace_back(chain.db().get_block_id_for_num(num));

   return ids;
} FC_CAPTURE_AND_RETHROW((blockchain_synopsis)) }

net::message p2p_plugin_impl::node_delegate::get_item(const net::item_id& id) {
   switch (id.item_type) {
   case net::block_message_type: {
      auto opt = chain.db().fetch_block_by_id(id.item_hash);
      EOS_ASSERT(opt, chain::unknown_block_exception, "No block found with id ${id}", ("id", id.item_hash));
      return net::block_message(std::move(*opt));
   }
   case net::trx_message_type:
      return net::trx_message(chain.db().get_recent_transaction(id.item_hash));
   default:
      FC_THROW("P2P asked for an item, but type of item is unknown. Item: ${item}", ("item", id));
   }
}

chain::chain_id_type p2p_plugin_impl::node_delegate::get_chain_id() const {
   return chain.db().get_chain_id();
}

std::vector<net::item_hash_t> p2p_plugin_impl::node_delegate::get_blockchain_synopsis(const net::item_hash_t& reference_point,
                                                                                      uint32_t number_of_blocks_after_reference_point) {
   using net::item_hash_t;
   using chain::block_id_type;
   using chain::block_header;

   std::vector<item_hash_t> synopsis;
   synopsis.reserve(30);
   uint32_t high_block_num;
   uint32_t non_fork_high_block_num;
   uint32_t low_block_num = chain.db().last_irreversible_block_num();
   std::vector<block_id_type> fork_history;

   if (reference_point != item_hash_t())
   {
      // the node is asking for a summary of the block chain up to a specified
      // block, which may or may not be on a fork
      // for now, assume it's not on a fork
      if (chain.block_is_on_preferred_chain(reference_point))
      {
         // reference_point is a block we know about and is on the main chain
         uint32_t reference_point_block_num = block_header::num_from_id(reference_point);
         assert(reference_point_block_num > 0);
         high_block_num = reference_point_block_num;
         non_fork_high_block_num = high_block_num;

         if (reference_point_block_num < low_block_num)
         {
            // we're on the same fork (at least as far as reference_point) but we've passed
            // reference point and could no longer undo that far if we diverged after that
            // block.  This should probably only happen due to a race condition where
            // the network thread calls this function, and then immediately pushes a bunch of blocks,
            // then the main thread finally processes this function.
            // with the current framework, there's not much we can do to tell the network
            // thread what our current head block is, so we'll just pretend that
            // our head is actually the reference point.
            // this *may* enable us to fetch blocks that we're unable to push, but that should
            // be a rare case (and correctly handled)
            low_block_num = reference_point_block_num;
         }
      }
      else
      {
         // block is a block we know about, but it is on a fork
         try
         {
            fork_history = chain.db().get_block_ids_on_fork(reference_point);
            // returns a vector where the last element is the common ancestor with the preferred chain,
            // and the first element is the reference point you passed in
            assert(fork_history.size() >= 2);

            if( fork_history.front() != reference_point )
            {
               edump( (fork_history)(reference_point) );
               assert(fork_history.front() == reference_point);
            }
            block_id_type last_non_fork_block = fork_history.back();
            fork_history.pop_back();  // remove the common ancestor
            boost::reverse(fork_history);

            if (last_non_fork_block == block_id_type()) // if the fork goes all the way back to genesis (does graphene's fork db allow this?)
               non_fork_high_block_num = 0;
            else
               non_fork_high_block_num = block_header::num_from_id(last_non_fork_block);

            high_block_num = non_fork_high_block_num + fork_history.size();
            assert(high_block_num == block_header::num_from_id(fork_history.back()));
         }
         catch (const fc::exception& e)
         {
            // unable to get fork history for some reason.  maybe not linked?
            // we can't return a synopsis of its chain
            elog("Unable to construct a blockchain synopsis for reference hash ${hash}: ${exception}", ("hash", reference_point)("exception", e));
            throw;
         }
         if (non_fork_high_block_num < low_block_num)
         {
            wlog("Unable to generate a usable synopsis because the peer we're generating it for forked too long ago "
                 "(our chains diverge after block #${non_fork_high_block_num} but only undoable to block #${low_block_num})",
                 ("low_block_num", low_block_num)
                 ("non_fork_high_block_num", non_fork_high_block_num));
            FC_THROW_EXCEPTION(eos::net::block_older_than_undo_history, "Peer is are on a fork I'm unable to switch to");
         }
      }
   }
   else
   {
      // no reference point specified, summarize the whole block chain
      high_block_num = chain.db().head_block_num();
      non_fork_high_block_num = high_block_num;
      if (high_block_num == 0)
         return synopsis; // we have no blocks
   }

   // at this point:
   // low_block_num is the block before the first block we can undo,
   // non_fork_high_block_num is the block before the fork (if the peer is on a fork, or otherwise it is the same as high_block_num)
   // high_block_num is the block number of the reference block, or the end of the chain if no reference provided

   // true_high_block_num is the ending block number after the network code appends any item ids it
   // knows about that we don't
   uint32_t true_high_block_num = high_block_num + number_of_blocks_after_reference_point;
   do
   {
      // for each block in the synopsis, figure out where to pull the block id from.
      // if it's <= non_fork_high_block_num, we grab it from the main blockchain;
      // if it's not, we pull it from the fork history
      if (low_block_num <= non_fork_high_block_num)
         synopsis.push_back(chain.db().get_block_id_for_num(low_block_num));
      else
         synopsis.push_back(fork_history[low_block_num - non_fork_high_block_num - 1]);
      low_block_num += (true_high_block_num - low_block_num + 2) / 2;
   }
   while (low_block_num <= high_block_num);

   return synopsis;
}

void p2p_plugin_impl::node_delegate::sync_status(uint32_t item_type, uint32_t item_count) {
   if (item_type == net::block_message_type)
      ilog("Sync status: ${blocks} left to sync", ("blocks", item_count));
}

void p2p_plugin_impl::node_delegate::connection_count_changed(uint32_t c) {
   ilog("${c} peers connected", ("c", c));
}

uint32_t p2p_plugin_impl::node_delegate::get_block_number(const net::item_hash_t& block_id) {
   return chain::block_header::num_from_id(block_id);
}

fc::time_point_sec p2p_plugin_impl::node_delegate::get_block_time(const net::item_hash_t& block_id) {
   if (auto block = chain.db().fetch_block_by_id(block_id))
      return block->timestamp;
   return fc::time_point_sec::min();
}

net::item_hash_t p2p_plugin_impl::node_delegate::get_head_block_id() const {
   return chain.db().head_block_id();
}

uint32_t p2p_plugin_impl::node_delegate::estimate_last_known_fork_from_git_revision_timestamp(uint32_t) const {
   // Forks? What are forks? All we have are spoons!
   return 0;
}

void p2p_plugin_impl::node_delegate::error_encountered(const string& message, const fc::oexception& error) {
   // Well this sucks... Guess I'll log it?
   elog("Error reported by P2P: ${msg}", ("msg", message)("error", error));
}

uint8_t p2p_plugin_impl::node_delegate::get_current_block_interval_in_seconds() const {
   return config::BlockIntervalSeconds;
}
////////////////////////////// End node_delegate Implementation //////////////////////////////

} // namespace eos
