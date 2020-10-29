#define BOOST_TEST_MODULE blockvault_sync_strategy
#include <eosio/chain/permission_object.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain_plugin/blockvault_sync_strategy.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace eosio::chain_apis;
using namespace eosio::blockvault;

class mock_blockvault_t : public block_vault_interface {
    virtual void append_proposed_block(watermark_t watermark, uint32_t lib, std::string_view block_content){}
    virtual void append_external_block(watermark_t watermark, std::string_view block_content){}
    virtual void propose_snapshot(watermark_t watermark, const char* snapshot_filename){}
    virtual void sync(std::string_view previous_block_id, sync_callback& callback){

    }


};

class mock_chain_t {
public:
    void startup_from_snapshot(std::shared_ptr<chain::istream_snapshot_reader>) {}
    bool accept_block( const chain::signed_block_ptr& block, const chain::block_id_type& id ){
        _block = block;
        _id = id;
        return _accept_block_rc;
    }

    bool _accept_block_rc;
    signed_block_ptr _block;
    block_id_type _id;

};

BOOST_AUTO_TEST_SUITE(blockvault_sync_strategy_tests)

BOOST_FIXTURE_TEST_CASE(newaccount_test, TESTER) { try {

    mock_chain_t chain;
    mock_blockvault_t bv;

    blockvault_sync_strategy<mock_chain_t> uut(bv, chain);
    uut.do_sync();

	//BOOST_TEST_REQUIRE( == true);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()

