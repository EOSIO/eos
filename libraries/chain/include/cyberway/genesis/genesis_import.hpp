#pragma once
#include <eosio/chain/controller.hpp>
#include <boost/filesystem.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

class genesis_import final {
public:
    genesis_import() = delete;
    genesis_import(const genesis_import&) = delete;

    genesis_import(const bfs::path& genesis_file, controller& ctrl);
    ~genesis_import();

    void import();

private:
    struct impl;
    std::unique_ptr<impl> _impl;
};


}} // cyberway::genesis
