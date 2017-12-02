#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/merkle.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

struct merkle_node {
   digest_type                   digest;
   optional<size_t>              left;
   optional<size_t>              right;
   optional<size_t>              parent;
};

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
   nodes.reserve(100);
   ids.reserve(100);
   for (const auto& blk_id: known_blocks) {
      nodes.emplace_back(merkle_node{blk_id});
      ids.push_back(blk_id);
   }

   size_t num_nodes = 100;
   size_t base = 0;
   while (num_nodes > 1) {
      size_t new_nodes = (num_nodes / 2) + (num_nodes % 2);
      for(size_t idx = 0; idx < new_nodes; idx++) {
         size_t l_idx = idx * 2;
         size_t r_idx = l_idx + 1;
         size_t left = base + l_idx;
         if (r_idx >= num_nodes) {
            nodes.emplace_back(
               merkle_node{
                  digest_type::hash(make_canonical_pair(nodes[left].digest, nodes[left].digest)),
                  left
               });
            nodes[left].parent.emplace(nodes.size() - 1);
         } else {
            size_t right = base + r_idx;
            nodes.emplace_back(
               merkle_node{
                  digest_type::hash(make_canonical_pair(nodes[left].digest, nodes[right].digest)),
                  left,
                  right
               });
            nodes[left].parent.emplace(nodes.size() - 1);
            nodes[right].parent.emplace(nodes.size() - 1);
         }
      }

      base += num_nodes;
      num_nodes = new_nodes;
   }

   BOOST_REQUIRE_EQUAL((std::string)nodes.back().digest, (std::string)merkle(ids));

   produce_block();

   BOOST_REQUIRE_EQUAL((std::string)nodes.back().digest, (std::string)last_block_header.block_mroot);

   for (int idx = 0; idx < 100; idx++) {
      size_t leaf = idx;
      digest_type current = ids.at(idx);
      vector<digest_type> path;
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


      BOOST_REQUIRE_EQUAL(std::string(current), std::string(last_block_header.block_mroot));

      /** UNCOMMENT TO PRODUCE PROOFS TO STD OUT
      std::cout << idx <<  ":" <<  std::string(known_blocks.at(idx)) << " = ";
      for (const auto& e: path) {
         std::cout << std::string(e) << " ";
      }

      std::cout << "-> " << std::string(last_block_header.block_mroot) << std::endl;
       */
   }

} FC_LOG_AND_RETHROW() } /// transfer_test


/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_AUTO_TEST_CASE( prove_action_in_block ) { try {

} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
