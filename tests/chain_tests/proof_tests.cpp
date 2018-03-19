#include <eosio/testing/tester.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/json.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

struct action_proof_data {
   account_name              receiver;
   scope_name                scope;
   action_name               name;
   bytes                     data;
   uint32_t                  region_id;
   uint32_t                  cycle_index;
   vector<data_access_info>  data_access;
};
FC_REFLECT(action_proof_data, (receiver)(scope)(name)(data)(region_id)(cycle_index)(data_access));


struct merkle_node {
   digest_type                   digest;
   optional<size_t>              left;
   optional<size_t>              right;
   optional<size_t>              parent;
};

digest_type process_merkle(vector<merkle_node>& nodes, vector<size_t> leaf_indices) {
   size_t num_nodes = leaf_indices.size();
   size_t base = 0;

   if (num_nodes == 0) {
      return digest_type();
   } else if (num_nodes == 1) {
      return nodes[leaf_indices.front()].digest;
   }

   while (num_nodes > 1) {
      size_t new_nodes = (num_nodes / 2) + (num_nodes % 2);
      for(size_t idx = 0; idx < new_nodes; idx++) {
         size_t l_idx = idx * 2;
         size_t r_idx = l_idx + 1;
         size_t left = leaf_indices.at(base + l_idx);
         if (r_idx >= num_nodes) {
            nodes.emplace_back(
               merkle_node{
                  digest_type::hash(make_canonical_pair(nodes[left].digest, nodes[left].digest)),
                  left
               });
            nodes[left].parent.emplace(nodes.size() - 1);
         } else {
            size_t right = leaf_indices.at(base + r_idx);
            nodes.emplace_back(
               merkle_node{
                  digest_type::hash(make_canonical_pair(nodes[left].digest, nodes[right].digest)),
                  left,
                  right
               });
            nodes[left].parent.emplace(nodes.size() - 1);
            nodes[right].parent.emplace(nodes.size() - 1);
         }

         leaf_indices.emplace_back(nodes.size() - 1);
      }

      base += num_nodes;
      num_nodes = new_nodes;
   }

   return nodes.back().digest;
}

auto get_proof_path(const vector<merkle_node>& nodes, size_t leaf) {
   vector<digest_type> path;
   digest_type current = nodes[leaf].digest;
   while (nodes[leaf].parent.valid()) {
      size_t parent = *nodes[leaf].parent;
      if (leaf == *nodes[parent].left) {
         if (nodes[parent].right.valid()) {
            size_t right = *nodes[parent].right;
            path.emplace_back(make_canonical_right(nodes[right].digest));
            current = digest_type::hash(make_canonical_pair(nodes[leaf].digest, nodes[right].digest));
         } else {
            path.emplace_back(make_canonical_right(nodes[leaf].digest));
            current = digest_type::hash(make_canonical_pair(nodes[leaf].digest, nodes[leaf].digest));
         }
      } else {
         size_t left = *nodes[parent].left;
         path.emplace_back(make_canonical_left(nodes[left].digest));
         current = digest_type::hash(make_canonical_pair(nodes[left].digest, nodes[leaf].digest));
      }

      leaf = parent;
   }

   return path;
}

digest_type apply_path(const digest_type& digest, const vector<digest_type>& path) {
   digest_type current = digest;

   for (const auto& p: path ) {
      if (is_canonical_left(p)) {
         current = digest_type::hash(make_canonical_pair(p, current));
      } else {
         current = digest_type::hash(make_canonical_pair(current, p));
      }
   }

   return current;
}

bool proof_is_valid(const digest_type& digest, const vector<digest_type>& path, const digest_type& expected_root) {
   return apply_path(digest, path) == expected_root;
}

BOOST_AUTO_TEST_SUITE(proof_tests)

BOOST_FIXTURE_TEST_CASE( prove_block_in_chain, tester ) { try {
   vector<block_id_type> known_blocks;
   known_blocks.reserve(100);
   block_header last_block_header;

    // register a callback on new blocks to record block information
   control->applied_block.connect([&](const block_trace& bt){
      known_blocks.emplace_back(bt.block.id());
      last_block_header = bt.block;
   });

   produce_blocks(100);

   vector<merkle_node> nodes;
   vector<digest_type> ids;
   vector<size_t> block_leaves;
   nodes.reserve(100);
   ids.reserve(100);
   for (const auto& blk_id: known_blocks) {
      nodes.emplace_back(merkle_node{blk_id});
      ids.push_back(blk_id);
      block_leaves.push_back(nodes.size() - 1);
   }

   digest_type block_mroot = process_merkle(nodes, move(block_leaves));

   BOOST_REQUIRE_EQUAL((std::string)block_mroot, (std::string)merkle(ids));

   produce_block();

   BOOST_REQUIRE_EQUAL((std::string)block_mroot, (std::string)last_block_header.block_mroot);

   for (int idx = 0; idx < 100; idx++) {
      vector<digest_type> path = get_proof_path(nodes, idx);

      BOOST_REQUIRE_EQUAL(true, proof_is_valid(nodes[idx].digest, path, last_block_header.block_mroot));

      /** UNCOMMENT TO PRODUCE PROOFS TO STD OUT
      std::cout << idx <<  ":" <<  std::string(known_blocks.at(idx)) << " = ";
      for (const auto& e: path) {
         std::cout << std::string(e) << " ";
      }

      std::cout << "-> " << std::string(last_block_header.block_mroot) << std::endl;
       */
   }

} FC_LOG_AND_RETHROW() } /// transfer_test


struct action_proof_info {
   action_trace trace;

   size_t action_leaf;
   size_t block_leaf;

   uint32_t cycle_index;
   uint32_t region_id;
};

/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_FIXTURE_TEST_CASE( prove_action_in_block, tester ) { try {
   vector<merkle_node> nodes;
   vector<size_t> block_leaves;
   vector<action_proof_info> known_actions;
   map<digest_type, digest_type> block_action_mroots;
   block_header last_block_header;
   block_id_type last_block_id;

   // register a callback on new blocks to record block information
   control->applied_block.connect([&](const block_trace& bt){
      nodes.emplace_back(merkle_node{bt.block.id()});
      size_t block_leaf = nodes.size() - 1;
      block_leaves.push_back(block_leaf);
      vector<size_t> shard_leaves;

      for (uint32_t r_idx = 0; r_idx < bt.region_traces.size(); r_idx++) {
         const auto& rt = bt.region_traces.at(r_idx);
         for (uint32_t c_idx = 0; c_idx < rt.cycle_traces.size(); c_idx++) {
            const auto& ct = rt.cycle_traces.at(c_idx);

            for (const auto& st: ct.shard_traces) {
               vector<size_t> action_leaves;

               for (const auto& tt: st.transaction_traces) {
                  for (const auto& at: tt.action_traces) {
                     digest_type::encoder enc;

                     auto a_data = action_proof_data {
                        at.receiver,
                        at.act.account,
                        at.act.name,
                        at.act.data,
                        at.region_id,
                        at.cycle_index,
                        at.data_access
                     };
                     fc::raw::pack(enc, a_data);
                     nodes.emplace_back(merkle_node{enc.result()});
                     size_t action_leaf = nodes.size() - 1;
                     known_actions.emplace_back(action_proof_info{at, action_leaf, block_leaf, c_idx, r_idx });
                     action_leaves.emplace_back(action_leaf);
                  }
               }

               if (action_leaves.size() > 0) {
                  process_merkle(nodes, move(action_leaves));
                  shard_leaves.emplace_back(nodes.size() - 1);
               }
            }
         }
      }

      digest_type action_mroot = process_merkle(nodes, move(shard_leaves));
      BOOST_REQUIRE_EQUAL((std::string)bt.block.action_mroot, (std::string)action_mroot);

      last_block_header = bt.block;
      last_block_id = bt.block.id();
      block_action_mroots[bt.block.id()] = bt.block.action_mroot;
   });

   create_accounts( { N(alice), N(bob), N(carol), N(david), N(elvis) });

   produce_blocks(50);

   push_dummy(N(alice), "AB");
   push_dummy(N(bob), "BC");
   push_dummy(N(carol), "CD");
   push_dummy(N(david), "DE");
   push_dummy(N(elvis), "EF");

   produce_blocks(50);
   digest_type block_mroot = process_merkle(nodes, move(block_leaves));

   produce_block();
   BOOST_REQUIRE_EQUAL((std::string)block_mroot, (std::string)last_block_header.block_mroot);

   /* UNCOMMENT TO PRODUCE PROOFS TO STDOUT
   std::cout << "Best Block ID: " << (std::string)last_block_id << std::endl;
   std::cout << "  Merkle Root: " << (std::string)last_block_header.block_mroot << std::endl;

   for(const auto& ai : known_actions) {
      auto block_path = get_proof_path(nodes, ai.action_leaf);
      auto chain_path = get_proof_path(nodes, ai.block_leaf);

      // prove action in block
      auto shard_root = apply_path(nodes[ai.action_leaf].digest, block_path);
      BOOST_REQUIRE_EQUAL((std::string)shard_root, (std::string)block_action_mroots[nodes[ai.block_leaf].digest]);

      // prove that block is part of the chain
      auto expected_block_mroot = apply_path(nodes[ai.block_leaf].digest, chain_path);
      BOOST_REQUIRE_EQUAL((std::string)expected_block_mroot, (std::string)last_block_header.block_mroot);

      std::cout << "Proof for Action:" << std::endl;
      std::cout << std::setw(14) << "reciever" << ":" << (std::string) ai.trace.receiver << std::endl;
      std::cout << std::setw(14) << "scope" << ":" << (std::string) ai.trace.act.scope << std::endl;
      std::cout << std::setw(14) << "name" << ":" << (std::string) ai.trace.act.name << std::endl;
      std::cout << std::setw(14) << "data" << ":" << fc::json::to_string(ai.trace.act.as<contracts::transfer>()) << std::endl;
      std::cout << std::setw(14) << "data_access" << ":" << fc::json::to_string(ai.trace.data_access) << std::endl;
      std::cout << std::setw(14) << "region" << ":" << ai.region_id << std::endl;
      std::cout << std::setw(14) << "cycle" << ":" << ai.cycle_index << std::endl;
      std::cout << std::setw(14) << "block" << ":" << (std::string)nodes[ai.block_leaf].digest << std::endl;
      std::cout << std::setw(14) << "am_root" << ":" << (std::string)block_action_mroots[nodes[ai.block_leaf].digest] << std::endl;
      std::cout << std::endl;

      auto a_data = action_proof_data {
         ai.trace.receiver,
         ai.trace.act.scope,
         ai.trace.act.name,
         ai.trace.act.data,
         ai.trace.region_id,
         ai.trace.cycle_index,
         ai.trace.data_access
      };
      auto action_data = fc::raw::pack(a_data);

      std::cout << "Action Hex: " << fc::to_hex(action_data) << std::endl;
      std::cout << "Action Hash: " << (std::string)nodes[ai.action_leaf].digest << std::endl;

      std::cout << "Action Path: ";
      for (const auto& p: block_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;

      std::cout << "Block Path: ";
      for (const auto& p: chain_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;
   }*/


} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
