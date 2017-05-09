/**
 * @file Contains support macros for the testcase helper macros. These macros are implementation details, and thus
 * should not be used directly. Use their frontends instead.
 */

#define MKDB1(name) testing_database name(*this); name.open();
#define MKDB2(name, id) testing_database name(*this, #id); name.open();
#define MKDBS_MACRO(x, y, name) Make_Database(name)

#define MKNET1(name) testing_network name;
#define MKNET2_MACRO(x, name, db) name.connect_database(db);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)

#define MKACCT_IMPL(db, name, creator, active, owner, recovery, deposit) \
   { \
      chain::SignedTransaction trx; \
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
      chain::SignedTransaction trx; \
      trx.emplaceMessage(#sender, "sys", vector<AccountName>{#recipient}, "Transfer", \
                                types::Transfer{#sender, #recipient, amount, memo}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define XFER4(db, sender, recipient, amount) XFER5(db, sender, recipient, amount, "")

#define MKPDCR3(db, owner, key) \
   { \
      chain::SignedTransaction trx; \
      trx.emplaceMessage(#owner, "sys", vector<AccountName>{}, "CreateProducer", \
                                types::CreateProducer{#owner, key}); \
      trx.expiration = db.head_block_time() + 100; \
      trx.set_reference_block(db.head_block_id()); \
      db.push_transaction(trx); \
   }
#define MKPDCR2(db, owner) \
   Make_Key(owner ## _producer); \
   MKPDCR3(db, owner, owner ## _producer_public_key);
