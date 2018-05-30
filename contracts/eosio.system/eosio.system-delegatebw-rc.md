# eosio.system delegatebw

This Ricardian contract for the system action *delegatebw* is legally binding and can be used in the event of a dispute. 

## delegatebw
    (account_name-from; 
     account_name-to; 
     asset-stake_net_quantity; 
     asset-stake_cpu_quantity; 
     bool:transfer)

_Intent: stake tokens for bandwidth and/or CPU and optionally transfer ownership_

As an authorized party I {{ signer }} wish to stake {{ asset-stake_cpu_quantity }} for CPU and {{ asset-stake_net_quantity }} for bandwidth from the liquid tokens of {{ account_name-from }} for the use of delegatee {{ account_name-to }}. 
  
    {{if bool:transfer }}
    
It is {{ bool:transfer }} that I wish these tokens to become immediately owned by the delegatee.
 
    {{/if}}

As signer I stipulate that, if I am not the beneficial owner of these tokens, I have proof that I’ve been authorized to take this action by their beneficial owner(s). 

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
