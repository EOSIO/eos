#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/json.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

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
   size_t shard_leaf;
   size_t cycle_leaf;
   size_t region_leaf;
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
      vector<size_t> region_leaves;

      for (uint32_t r_idx = 0; r_idx < bt.region_traces.size(); r_idx++) {
         const auto& rt = bt.region_traces.at(r_idx);
         nodes.emplace_back(merkle_node());
         size_t region_leaf = nodes.size() - 1;
         vector<size_t> cycle_leaves;

         for (uint32_t c_idx = 0; c_idx < rt.cycle_traces.size(); c_idx++) {
            const auto& ct = rt.cycle_traces.at(c_idx);
            nodes.emplace_back(merkle_node());
            size_t cycle_leaf = nodes.size() - 1;
            vector<size_t> shard_leaves;

            for (const auto& st: ct.shard_traces) {
               nodes.emplace_back(merkle_node());
               size_t shard_leaf = nodes.size() - 1;
               vector<size_t> action_leaves;

               for (const auto& tt: st.transaction_traces) {
                  for (const auto& at: tt.action_traces) {
                     digest_type::encoder enc;

                     fc::raw::pack(enc, at.receiver);
                     fc::raw::pack(enc, at.act.scope);
                     fc::raw::pack(enc, at.act.name);
                     fc::raw::pack(enc, at.act.data);
                     fc::raw::pack(enc, at.data_access);

                     nodes.emplace_back(merkle_node{enc.result()});
                     size_t action_leaf = nodes.size() - 1;
                     known_actions.emplace_back(action_proof_info{at, action_leaf, shard_leaf, cycle_leaf, region_leaf, block_leaf, c_idx, r_idx });
                     action_leaves.emplace_back(action_leaf);
                  }
               }

               nodes[shard_leaf].digest = process_merkle(nodes, move(action_leaves));
               shard_leaves.emplace_back(shard_leaf);
            }

            digest_type cycle_root = process_merkle(nodes, move(shard_leaves));
            digest_type::encoder enc;
            fc::raw::pack(enc, c_idx);
            fc::raw::pack(enc, cycle_root);
            nodes[cycle_leaf].digest = enc.result();
            cycle_leaves.emplace_back(cycle_leaf);
         }

         digest_type region_root = process_merkle(nodes, move(cycle_leaves));
         digest_type::encoder enc;
         fc::raw::pack(enc, r_idx);
         fc::raw::pack(enc, region_root);
         nodes[region_leaf].digest = enc.result();
         region_leaves.emplace_back(region_leaf);
      }

      digest_type action_mroot = process_merkle(nodes, move(region_leaves));
      BOOST_REQUIRE_EQUAL((std::string)bt.block.action_mroot, (std::string)action_mroot);

      last_block_header = bt.block;
      last_block_id = bt.block.id();
      block_action_mroots[bt.block.id()] = bt.block.action_mroot;
   });

   produce_blocks(50);

   transfer( N(inita), N(initb), "1.0000 EOS", "memo" );
   transfer( N(initb), N(initc), "1.0000 EOS", "memo" );
   transfer( N(initc), N(initd), "1.0000 EOS", "memo" );
   transfer( N(initd), N(inite), "1.0000 EOS", "memo" );
   transfer( N(inite), N(inita), "1.0000 EOS", "memo" );

   produce_blocks(50);
   digest_type block_mroot = process_merkle(nodes, move(block_leaves));

   produce_block();
   BOOST_REQUIRE_EQUAL((std::string)block_mroot, (std::string)last_block_header.block_mroot);

   std::cout << "Best Block ID: " << (std::string)last_block_id << std::endl;
   std::cout << "  Merkle Root: " << (std::string)last_block_header.block_mroot << std::endl;

   for(const auto& ai : known_actions) {
      auto shard_path = get_proof_path(nodes, ai.action_leaf);
      auto cycle_path = get_proof_path(nodes, ai.shard_leaf);
      auto region_path = get_proof_path(nodes, ai.cycle_leaf);
      auto block_path = get_proof_path(nodes, ai.region_leaf);
      auto chain_path = get_proof_path(nodes, ai.block_leaf);

      // prove action in shard
      auto shard_root = apply_path(nodes[ai.action_leaf].digest, shard_path);
      BOOST_REQUIRE_EQUAL((std::string)shard_root, (std::string)nodes[ai.shard_leaf].digest);

      // prove its in the cycle
      auto cycle_root = apply_path(shard_root, cycle_path);
      auto cycle_digest = digest_type::hash(std::make_pair(ai.cycle_index, cycle_root));
      BOOST_REQUIRE_EQUAL((std::string)cycle_digest, (std::string)nodes[ai.cycle_leaf].digest);

      // prove that cycle is in the region
      auto region_root = apply_path(cycle_digest, region_path);
      auto region_digest = digest_type::hash(std::make_pair(ai.region_id, region_root));
      BOOST_REQUIRE_EQUAL((std::string)region_digest, (std::string)nodes[ai.region_leaf].digest);

      // prove that region is in the correct blocks action_mroot
      auto action_mroot = apply_path(region_digest, block_path);
      BOOST_REQUIRE_EQUAL((std::string)action_mroot, (std::string)block_action_mroots[nodes[ai.block_leaf].digest]);

      // prove that block is part of the chain
      auto expected_block_mroot = apply_path(nodes[ai.block_leaf].digest, chain_path);
      BOOST_REQUIRE_EQUAL((std::string)expected_block_mroot, (std::string)last_block_header.block_mroot);

      std::cout << "Proof for Action:" << std::endl;
      std::cout << std::setw(10) << "reciever" << ":" << (std::string) ai.trace.receiver << std::endl;
      std::cout << std::setw(10) << "scope" << ":" << (std::string) ai.trace.act.scope << std::endl;
      std::cout << std::setw(10) << "name" << ":" << (std::string) ai.trace.act.name << std::endl;
      std::cout << std::setw(10) << "data" << ":" << fc::json::to_string(ai.trace.act.as<contracts::transfer>()) << std::endl;
      std::cout << std::setw(10) << "block" << ":" << (std::string)nodes[ai.block_leaf].digest << std::endl;
      std::cout << std::setw(10) << "region" << ":" << ai.region_id << std::endl;
      std::cout << std::setw(10) << "cycle" << ":" << ai.cycle_index << std::endl;
      std::cout << std::endl;
      std::cout << "Action Hash: " << (std::string)nodes[ai.action_leaf].digest << std::endl;
      std::cout << "Shard Path: ";
      for (const auto& p: shard_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;

      std::cout << "Cycle Path: ";
      for (const auto& p: cycle_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;

      std::cout << "Region Path: ";
      for (const auto& p: region_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;

      std::cout << "Block Path: ";
      for (const auto& p: block_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;

      std::cout << "Chain Path: ";
      for (const auto& p: chain_path) {
         std::cout << (std::string)p << " ";
      }
      std::cout << std::endl;
   }


} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
