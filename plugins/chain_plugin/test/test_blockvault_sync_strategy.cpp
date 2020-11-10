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

struct mock_chain_t {
    void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown, std::shared_ptr<istream_snapshot_reader> reader) {
       _shutdown = shutdown;
       _check_shutdown = check_shutdown;
       _reader = reader;
       _startup_reader_called = true;
    }

    void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown) {
        _shutdown = shutdown;
        _check_shutdown = check_shutdown;
        _startup_no_reader_called = true;
    }

    std::function<void()> _shutdown;
    std::function<bool()> _check_shutdown;
    std::shared_ptr<istream_snapshot_reader> _reader;

    bool _startup_no_reader_called;
    bool _startup_reader_called;
};

struct mock_blockvault_t : public block_vault_interface {
    virtual void async_propose_constructed_block(watermark_t watermark, uint32_t lib,
                                                 eosio::chain::signed_block_ptr block,
                                                 std::function<void(bool)>      handler) override {}
    virtual void async_append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block,
                                             std::function<void(bool)> handler)  override {}

    virtual bool propose_snapshot(watermark_t watermark, const char* snapshot_filename) override {return true;}
    virtual void sync(const eosio::chain::block_id_type* previous_block_id, sync_callback& callback) override {
       _previous_block_id = previous_block_id;
    }

    const eosio::chain::block_id_type* _previous_block_id;
};

struct mock_chain_plugin_t {
    bool incoming_block_sync_method( const chain::signed_block_ptr& block, const chain::block_id_type& id ){
        _block = block;
        _id = id;
        return _accept_block_rc;
    }

    bool _accept_block_rc;
    signed_block_ptr _block;
    block_id_type _id;
    std::unique_ptr<mock_chain_t> chain;
};

BOOST_AUTO_TEST_SUITE(blockvault_sync_strategy_tests)

BOOST_FIXTURE_TEST_CASE(empty_previous_block_id_test, TESTER) { try {

    mock_chain_plugin_t chain;
    mock_blockvault_t bv;

    auto shutdown = [](){ return false; };
    auto check_shutdown = [](){ return false; };

    blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, chain, shutdown, check_shutdown);
    uut.do_sync();

	BOOST_TEST(nullptr == bv._previous_block_id);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(nonempty_previous_block_id_test, TESTER) { try {

    mock_chain_plugin_t chain;
    mock_blockvault_t bv;
    chain.chain = std::make_unique<mock_chain_t>();

    auto shutdown = [](){ return false; };
    auto check_shutdown = [](){ return false; };
    std::string bid_hex("deadbabe000000000000000000000000000000000000000000000000deadbeef");
    chain::block_id_type bid(bid_hex);

    blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, chain, shutdown, check_shutdown, bid);
    uut.do_sync();

    BOOST_TEST(*bv._previous_block_id == bid);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(on_block_no_snapshot, TESTER) { try {

    mock_chain_plugin_t chain;
    mock_blockvault_t bv;
    chain.chain = std::make_unique<mock_chain_t>();

    auto shutdown = [](){ return false; };
    auto check_shutdown = [](){ return false; };
    blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, chain, shutdown, check_shutdown);
    auto b = produce_empty_block();

    // std::stringstream ss;
    // fc::raw::pack(ss, b) ;
    //std::string block_string = ss.str();

    uut.on_block(b);
    BOOST_TEST(chain.chain->_reader == nullptr);
    BOOST_TEST(chain.chain->_startup_no_reader_called);
    BOOST_TEST(!chain.chain->_startup_reader_called);
    BOOST_TEST(chain._block->calculate_id() == b->calculate_id());
    BOOST_TEST(chain._block->calculate_id() == chain._id);

//BOOST_TEST(bv._previous_block_id == bid_hex);

} FC_LOG_AND_RETHROW() }
BOOST_AUTO_TEST_SUITE_END()

