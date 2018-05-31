# eosio.system undelegatebw

## undelegatebw 
    (account_name-from; 
     account_name-to; 
     asset-unstake_net_quantity; 
     asset-unstake_cpu_quantity)

_Intent: unstake tokens from bandwidth_

As an authorized party I {{ signer }} wish to unstake {{ asset-unstake_cpu_quantity }} from CPU and {{ asset-unstake_net_quantity }} from bandwidth from the tokens owned by {{ account_name-from }} previously delegated for the use of delegatee {{ account_name-to }}. 

If I as signer am not the beneficial owner of these tokens I stipulate I have proof that I’ve been authorized to take this action by their beneficial owner(s). 
