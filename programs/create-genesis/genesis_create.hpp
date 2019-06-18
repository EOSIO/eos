#pragma once
#include "genesis_info.hpp"
#include "export_info.hpp"
#include <eosio/chain/genesis_state.hpp>
#include <fc/crypto/sha256.hpp>
#include <boost/filesystem.hpp>


namespace cyberway { namespace genesis {

FC_DECLARE_EXCEPTION(genesis_exception, 9000000, "genesis create exception");

using namespace eosio::chain;
namespace bfs = boost::filesystem;
using mvo = fc::mutable_variant_object;

struct contract_abicode {
    bool update;
    bytes abi;
    bytes code;
    fc::sha256 code_hash;
    fc::sha256 abi_hash;
};
using contracts_map = std::map<name, contract_abicode>;

class genesis_create final {
public:
    genesis_create(const genesis_create&) = delete;
    genesis_create();
    ~genesis_create();

    void read_state(const bfs::path& state_file);
    void write_genesis(const bfs::path& out_file, const genesis_info&, const genesis_state&, const contracts_map&);

    // ee interface
    const genesis_info& get_info() const;
    const genesis_state& get_conf() const;
    const export_info& get_exp_info() const;
    name name_by_idx(acc_idx idx) const;

private:
    struct genesis_create_impl;
    std::unique_ptr<genesis_create_impl> _impl;
};


}} // cyberway::genesis
