#pragma once
#include <eosio/chain/controller.hpp>

#include <boost/filesystem.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

class genesis_read final {
public:
    genesis_read() = delete;
    genesis_read(const genesis_read&) = delete;

    genesis_read(const bfs::path& genesis_file, controller& ctrl, time_point genesis_ts);
    ~genesis_read();

    void read();

private:
    struct genesis_read_impl;
    std::unique_ptr<genesis_read_impl> _impl;
};


}} // cyberway::genesis
