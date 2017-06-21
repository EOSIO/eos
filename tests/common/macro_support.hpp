/**
 * @file Contains support macros for the testcase helper macros. These macros are implementation details, and thus
 * should not be used directly. Use their frontends instead.
 */

#define MKDB1(name) \
   chainbase::database name ## _db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir() / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_database name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this);
#define MKDB2(name, id) \
   chainbase::database name ## _db(get_temp_dir(#id), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir(#id) / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_database name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this);
#define MKDBS_MACRO(x, y, name) Make_Database(name)

#define MKNET1(name) testing_network name;
#define MKNET2_MACRO(x, name, db) name.connect_database(db);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)

#define MKACCT_IMPL(db, name, creator, active, owner, recovery, deposit) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#creator, config::SystemContractName, \
                         vector<types::AccountName>{config::StakedBalanceContractName, \
                                                    config::EosContractName}, "CreateAccount", \
                         types::CreateAccount{#creator, #name, owner, active, recovery, deposit}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define MKACCT2(db, name) \
   Make_Key(name) \
   MKACCT_IMPL(db, name, init0, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), Account_Authority(init0), Asset(100))
#define MKACCT3(db, name, creator) \
   Make_Key(name) \
   MKACCT_IMPL(db, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), Account_Authority(creator), Asset(100))
#define MKACCT4(db, name, creator, deposit) \
   Make_Key(name) \
   MKACCT_IMPL(db, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), Account_Authority(creator), deposit)
#define MKACCT5(db, name, creator, deposit, owner) \
   Make_Key(name) \
   MKACCT_IMPL(db, name, creator, owner, Key_Authority(name ## _public_key), Account_Authority(creator), deposit)
#define MKACCT6(db, name, creator, deposit, owner, active) \
   MKACCT_IMPL(db, name, creator, owner, active, Account_Authority(creator), deposit)
#define MKACCT7(db, name, creator, deposit, owner, active, recovery) \
   MKACCT_IMPL(db, name, creator, owner, active, recovery, deposit)

#define XFER5(db, sender, recipient, amount, memo) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#sender, config::EosContractName, vector<AccountName>{#recipient}, "Transfer", \
                                types::Transfer{#sender, #recipient, amount, memo}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define XFER4(db, sender, recipient, amount) XFER5(db, sender, recipient, amount, "")

#define STAKE4(db, sender, recipient, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#sender, config::EosContractName, vector<AccountName>{config::StakedBalanceContractName}, \
                         "TransferToLocked", types::TransferToLocked{#sender, #recipient, amount}); \
      if (std::string(#sender) != std::string(#recipient)) { \
         trx.messages.front().notify.emplace_back(#recipient); \
         boost::sort(trx.messages.front().notify); \
      } \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define STAKE3(db, account, amount) STAKE4(db, account, account, amount)

#define BEGIN_UNSTAKE3(db, account, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#account, config::StakedBalanceContractName, vector<AccountName>{}, \
                         "StartUnlockEos", types::StartUnlockEos{#account, amount}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }

#define FINISH_UNSTAKE3(db, account, amount) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#account, config::StakedBalanceContractName, vector<AccountName>{config::EosContractName}, \
                         "ClaimUnlockedEos", types::ClaimUnlockedEos{#account, amount}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }

#define MKPDCR4(db, owner, key, cfg) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#owner, config::StakedBalanceContractName, vector<AccountName>{}, "CreateProducer", \
                         types::CreateProducer{#owner, key, cfg}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define MKPDCR3(db, owner, key) MKPDCR4(db, owner, key, BlockchainConfiguration{});
#define MKPDCR2(db, owner) \
   Make_Key(owner ## _producer); \
   MKPDCR4(db, owner, owner ## _producer_public_key, BlockchainConfiguration{});

#define APPDCR4(db, voter, producer, approved) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#voter, config::StakedBalanceContractName, vector<AccountName>{}, "ApproveProducer", \
                         types::ApproveProducer{#producer, approved? 1 : 0}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }

#define UPPDCR4(db, owner, key, cfg) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(owner, config::StakedBalanceContractName, vector<AccountName>{}, "UpdateProducer", \
                                types::UpdateProducer{owner, key, cfg}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define UPPDCR3(db, owner, key) UPPDCR4(db, owner, key, db.get_producer(owner).configuration)
