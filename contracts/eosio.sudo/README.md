eosio.sudo
--------

Actions:
The naming convention is codeaccount::actionname followed by a list of parameters.

Execute a transaction while bypassing regular authorization checks (requires authorization of eosio.sudo which needs to be a privileged account)
## eosio.sudo::exec    executer trx
   - **executer** account executing the transaction
   - **trx** transaction to execute

   Deferred transaction RAM usage is billed to 'executer'

Cleos usage example.

TODO: Fill in cleos usage example.
