#pragma once

#include <vector>
#include <odb/nullable.hxx>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#pragma db model version(1, 1)

#pragma db namespace session pointer(std::shared_ptr)
namespace eosio {

using odb::nullable;

using std::string;
// typedef string checksum;
// #pragma db value(checksum) type("CHAR(32)")

typedef uint64_t name_t;

typedef std::vector<char> bytes;
#pragma db value(bytes) type("BLOB")

typedef boost::posix_time::ptime datetime;
#pragma db value(datetime) type("DATETIME(6)")

#pragma db object
struct Stats {
    #pragma db id
    unsigned id_ = 0; // global single row
    uint64_t tx_count_;
    uint64_t action_count_;
    uint64_t context_free_action_count_;
};

#pragma db object
struct Block {
    #pragma db id type("BINARY(32)")
    bytes id_;
    unsigned num_;

    datetime timestamp_;
    /* name_t producer_;
    uint16_t confirmed_;
    #pragma db type("BINARY(32)")
    bytes prev_id_;
    #pragma db type("BINARY(32)")
    bytes tx_mroot_;
    #pragma db type("BINARY(32)")
    bytes action_mroot_;
    uint32_t schedule_version_;
    #pragma db null
    nullable<bytes> new_producers_;
    #pragma db null
    nullable<bytes> header_extensions_;

    #pragma db type("VARCHAR(256)")
    string producer_signature;

    #pragma db null
    nullable<bytes> block_extensions_; */

    #pragma db type("MEDIUMBLOB")
    bytes block_;

    uint32_t tx_count_{};
    uint32_t action_count_{};
    uint32_t context_free_action_count_{};

    datetime created_at_;

    #pragma db index member(num_)
    #pragma db index member(timestamp_)
    #pragma db index member(created_at_)
};

#pragma db object
struct Transaction {
    #pragma db id type("BINARY(32)")
    bytes id_;

    #pragma db type("BINARY(32)")
    bytes block_id_;
    uint32_t block_num_;
    datetime block_time_;

    uint16_t block_seq_; // the sequence number of this transaction in its block

    /*
    #pragma db null
    nullable<bytes> transaction_header_; // whole transaction_header
    #pragma db null
    nullable<bytes> transaction_extensions_; // in transaction
    #pragma db null
    nullable<bytes> signatures_; // in signed_transaction
    #pragma db null
    nullable<bytes> context_free_data_; // in signed_transaction
    */

    uint32_t action_count_{};
    uint32_t context_free_action_count_{};

    #pragma db index member(block_id_)
    #pragma db index("block_num_seq_i") members(block_num_, block_seq_)
    #pragma db index member(block_time_)
};

enum TransactionStatus {
    executed, soft_fail, hard_fail, delayed, expired, unknown
};

#pragma db object
struct TransactionTrace { // new ones will override old ones, typically when status is changed
    #pragma db id type("BINARY(32)")
    bytes id_;

    // int64_t elapsed_us_;
    // uint64_t net_usage_; // useless
    bool scheduled_;

    // transaction receipt
    TransactionStatus status_;
    unsigned net_usage_words_;
    uint32_t cpu_usage_us_;

    #pragma db type("TEXT") null
    nullable<string> exception_;

    #pragma db index member(status_)
};

#pragma db object
struct Action {
    #pragma db id
    uint64_t global_seq_; // the global sequence number of this action
    uint64_t account_seq_; // the sequence number of this action for its account

    uint64_t parent_seq_; // parent action trace global sequence number, only for inline traces

    name_t account_; // account name
    name_t name_; // action name
    #pragma db null
    nullable<bytes> auth_; // binary serialization of authorization array of permission_level
    #pragma db type("MEDIUMBLOB")
    bytes data_; // payload

    name_t receiver_; // where this action is executed on; may not be equal with `account_`, such as from notification

    #pragma db null
    nullable<bytes> auth_seq_;
    unsigned code_seq_;
    unsigned abi_seq_;
    // #pragma db type("BINARY(32)")
    // bytes digest_; // useless

    #pragma db type("BINARY(32)")
    bytes tx_id_; // the transaction that generated this action
    // int64_t elapsed_us_; // useless
    // uint64_t cpu_usage_; // useless
    #pragma db type("TEXT") null
    nullable<string> console_;
    // uint64_t total_cpu_usage_; // useless

    #pragma db index member(account_seq_)
    #pragma db index member(parent_seq_)
    #pragma db index member(account_)
    #pragma db index member(name_)
    #pragma db index member(receiver_)
    #pragma db index member(tx_id_)
};

/*
#pragma db object
struct Token { // readonly
    #pragma db id type("BINARY(16)")
    bytes id_;

    #pragma db type("VARCHAR(13)")
    string contract_; // contract that issue this token
    #pragma db type("VARCHAR(8)")
    string symbol_; // token symbol name, e.g., EOS
    uint8_t precision_;

    datetime createdat_;
};
*/

#pragma db object
struct TokenTransfer { // token transfer with action name 'transfer', on contract eosio.token or other customized contract
    #pragma db id
    uint64_t action_global_seq_;

    #pragma db type("VARCHAR(8)")
    string symbol_; // token symbol name, e.g., EOS
    uint8_t precision_;
    int64_t amount_;

    name_t from_;
    name_t to_;
    #pragma db type("TEXT") null
    nullable<string> memo_;

    #pragma db type("BINARY(32)")
    bytes tx_id_; // the transaction that generated this action

    #pragma db index member(symbol_)
    #pragma db index member(from_)
    #pragma db index member(to_)
    #pragma db index member(tx_id_)
};

using StatsPtr = std::shared_ptr<Stats>;
using BlockPtr = std::shared_ptr<Block>;
using TransactionPtr = std::shared_ptr<Transaction>;
using TransactionTracePtr = std::shared_ptr<TransactionTrace>;
using ActionPtr = std::shared_ptr<Action>;

using TokenTransferPtr = std::shared_ptr<TokenTransfer>;

}
