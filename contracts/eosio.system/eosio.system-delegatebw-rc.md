# Action - `{{ delegatebw }}`

## Description

The intent of the `{{ delegatebw }}` action is to stake tokens for bandwidth and/or CPU and optionally transfer ownership.

As an authorized party I {{ signer }} wish to stake {{ stake_cpu_quantity }} for CPU and {{ stake_net_quantity }} for bandwidth from the liquid tokens of {{ from }} for the use of delegatee {{ to }}. 
  
    {{if transfer }}
    
It is {{ transfer }} that I wish these tokens to become immediately owned by the delegatee.
 
    {{/if}}

As signer I stipulate that, if I am not the beneficial owner of these tokens, I have proof that I’ve been authorized to take this action by their beneficial owner(s). 
