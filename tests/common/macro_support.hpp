/**
 * @file Contains support macros for the testcase helper macros. These macros are implementation details, and thus
 * should not be used directly. Use their frontends instead.
 */

#define MKDB1(name) \
   chainbase::database name ## _db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir() / "blocklog"); \
   fork_database name ## _fdb; \
   testing_database name(name ## _db, name ## _fdb, name ## _log, *this);
#define MKDB2(name, id) \
   chainbase::database name ## _db(get_temp_dir(#id), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir(#id) / "blocklog"); \
   fork_database name ## _fdb; \
   testing_database name(name ## _db, name ## _fdb, name ## _log, *this);
#define MKDBS_MACRO(x, y, name) Make_Database(name)

#define MKNET1(name) testing_network name;
#define MKNET2_MACRO(x, name, db) name.connect_database(db);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)

#define MKACCT_IMPL(db, name, creator, active, owner, recovery, deposit) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#creator, "sys", vector<AccountName>{}, "CreateAccount", \
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
      trx.emplaceMessage(#sender, "sys", vector<AccountName>{#recipient}, "Transfer", \
                                types::Transfer{#sender, #recipient, amount, memo}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define XFER4(db, sender, recipient, amount) XFER5(db, sender, recipient, amount, "")

#define MKPDCR4(db, owner, key, config) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(#owner, "sys", vector<AccountName>{}, "CreateProducer", \
                                types::CreateProducer{#owner, key, config}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define MKPDCR3(db, owner, key) MKPDCR4(db, owner, key, BlockchainConfiguration{});
#define MKPDCR2(db, owner) \
   Make_Key(owner ## _producer); \
   MKPDCR4(db, owner, owner ## _producer_public_key, BlockchainConfiguration{});

#define UPPDCR4(db, owner, key, config) \
   { \
      eos::chain::SignedTransaction trx; \
      trx.emplaceMessage(owner, "sys", vector<AccountName>{}, "UpdateProducer", \
                                types::UpdateProducer{owner, key, config}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define UPPDCR3(db, owner, key) UPPDCR4(db, owner, key, db.get_model().get_producer(owner).configuration)
