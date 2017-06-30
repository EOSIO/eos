/**
 * @file Contains support macros for the testcase helper macros. These macros are implementation details, and thus
 * should not be used directly. Use their frontends instead.
 */

#define MKCHAIN1(name) \
   chainbase::database name ## _db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir() / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_blockchain name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this);
#define MKCHAIN2(name, id) \
   chainbase::database name ## _db(get_temp_dir(#id), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir(#id) / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_blockchain name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this);
#define MKCHAINS_MACRO(x, y, name) Make_Blockchain(name)

#define MKNET1(name) testing_network name;
#define MKNET2_MACRO(x, name, chain) name.connect_blockchain(chain);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)

#define MKACCT_IMPL(chain, name, creator, active, owner, recovery, deposit) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#creator, config::SystemContractName, \
                         vector<types::AccountName>{config::StakedBalanceContractName, \
                                                    config::EosContractName}, "newaccount", \
                         types::newaccount{#creator, #name, owner, active, recovery, deposit}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }
#define MKACCT2(chain, name) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, inita, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(inita), Asset(100))
#define MKACCT3(chain, name, creator) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(creator), Asset(100))
#define MKACCT4(chain, name, creator, deposit) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(creator), deposit)
#define MKACCT5(chain, name, creator, deposit, owner) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, owner, Key_Authority(name ## _public_key), Account_Authority(creator), deposit)
#define MKACCT6(chain, name, creator, deposit, owner, active) \
   MKACCT_IMPL(chain, name, creator, owner, active, Account_Authority(creator), deposit)
#define MKACCT7(chain, name, creator, deposit, owner, active, recovery) \
   MKACCT_IMPL(chain, name, creator, owner, active, recovery, deposit)

#define XFER5(chain, sender, recipient, amount, memo) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#sender, config::EosContractName, vector<AccountName>{#recipient}, "transfer", \
                                types::transfer{#sender, #recipient, amount, memo}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }
#define XFER4(chain, sender, recipient, amount) XFER5(chain, sender, recipient, amount, "")

#define STAKE4(chain, sender, recipient, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#sender, config::EosContractName, vector<AccountName>{config::StakedBalanceContractName}, \
                         "lock", types::lock{#sender, #recipient, amount}); \
      if (std::string(#sender) != std::string(#recipient)) { \
         trx.messages.front().notify.emplace_back(#recipient); \
         boost::sort(trx.messages.front().notify); \
      } \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }
#define STAKE3(chain, account, amount) STAKE4(chain, account, account, amount)

#define BEGIN_UNSTAKE3(chain, account, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#account, config::StakedBalanceContractName, vector<AccountName>{}, \
                         "unlock", types::unlock{#account, amount}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }

#define FINISH_UNSTAKE3(chain, account, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#account, config::StakedBalanceContractName, vector<AccountName>{config::EosContractName}, \
                         "claim", types::claim{#account, amount}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }

#define MKPDCR4(chain, owner, key, cfg) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#owner, config::StakedBalanceContractName, vector<AccountName>{}, "setproducer", \
                         types::setproducer{#owner, key, cfg}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }
#define MKPDCR3(chain, owner, key) MKPDCR4(chain, owner, key, BlockchainConfiguration{});
#define MKPDCR2(chain, owner) \
   Make_Key(owner ## _producer); \
   MKPDCR4(chain, owner, owner ## _producer_public_key, BlockchainConfiguration{});

#define APPDCR4(chain, voter, producer, approved) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#voter, config::StakedBalanceContractName, vector<AccountName>{}, "okproducer", \
                         types::okproducer{#producer, approved? 1 : 0}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }

#define UPPDCR4(chain, owner, key, cfg) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(owner, config::StakedBalanceContractName, vector<AccountName>{}, "setproducer", \
                                types::setproducer{owner, key, cfg}); \
      trx.expiration = chain.head_block_time() + 100; \
      trx.set_reference_block(chain.head_block_id()); \
      chain.push_transaction(trx); \
   }
#define UPPDCR3(chain, owner, key) UPPDCR4(chain, owner, key, chain.get_producer(owner).configuration)
