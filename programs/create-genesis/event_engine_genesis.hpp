#pragma once

#include <boost/filesystem.hpp>
#include <fc/crypto/sha256.hpp>
#include <cyberway/genesis/ee_genesis_serializer.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfp = boost::filesystem;

class event_engine_genesis final {
public:
    event_engine_genesis(const event_engine_genesis&) = delete;
    event_engine_genesis();

    void start(const bfp::path& ee_directory, const fc::sha256& hash);
    void finalize();

    ~event_engine_genesis();

public:
    ee_genesis_serializer usernames;
    ee_genesis_serializer balances;
};


}} // cyberway::genesis
