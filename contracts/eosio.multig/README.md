EOSIO MultiSig
--------------

This contract is a **priviledged** contract designed to facilitate incremental,
on-chain, permission granting. This is in contrast to off-chain multisig where
all signatures are gathered prior to broadcasting the transaction.

One of the primary benefits of on-chain incremental multi-sig is that there is
no dependence on a 3rd party facilitator and it is possible to grant permissions
on highly nested / complex transactions. With the use of TaPoS it may be challenging
to get all of the parties to sign a transaction in a timely maner. Additionally, it
is possible for parties to retract the granted permissions

Actions
-------

propose [proposer] HEXTRX effective_time
   - requires the proposer to allocate storage for the transaction, storage will
   be freed when the transaction expires or is successfully executed
   - the proposed transaction will be stored in the eosio.multisig/$proposer/proposal[trxid[0]] table[row]
   - after approval has been granted it will be scheduled at effective time
       - it may be canceled by any party removing an authority

reject [permission_level] proposer proposalid 
   - requires rejector@permission_level or higher authority
   - this is separate from approve so that rejections can have different permissions
      from acceptance.

approve [permission_level] proposer proposalid
   - adds or removes approver@permission_level from approval table
   - requires approver@permission_level or higher authority

cancel [proposer] [trxid]
   - a transaction that was proposed, may only be performed by proposer


Priviledged
-----------
This contract requires the authority to generate scheduled transactions with
arbitrary authority levels. Normally a contract can only generate transactions
with the authority of its own contract. This contract can be an exception because it
is blessed by the block producers as "trusted" not to abuse the power and because it
takes 2/3 of producers to update it.
