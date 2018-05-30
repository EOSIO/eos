# eosio.system regproducer

I, {{producer}}, hereby nominate myself for consideration as an elected block producer. 

If {{producer}} is selected to produce blocks by the eosio contract, I will sign blocks with {{producer_key}} and I hereby certify that I will keep this key secret and secure. 

If {{producer}} is unable to perform obligations under this contract I will resign my position by setting by resubmitting this contract with
the null producer key.  

{{producer}} hereby agrees to only use {{producer_key}} to sign messages under the following scenarios:

1. proposing a block at the time appointed by the block scheduling algorithm 
2. pre-confirmation for  block produced by a 3rd party when it is deemed valid 
3. confirming a block for which {{producer}} has received 2/3+ pre-confirmation messages from other producers

I hereby accept liability for any and all provable damages that result from:

1. signing two different block proposals with the same timestamp with {{producer_key}
2. signing two different block proposals with the same block number with {{producer_key}
3. signing any block proposal which builds off of an objectively bad block
4. signing a pre-confirmation for an objectively invalid block according to deterministic blockchain rules
5. signing a confirmation for a block for which I do not possess pre-confirmation messages from 2/3+ other producers

I hereby agree that double-signing for a timestamp or block number in concert with 2 or more other producers shall automatically be deemed malicious and subject to a fine equal to the past year of compensation received and imediate disqualification from being a producer. An exception may be made if {{producer}} can demonstrate that the double-signing occured due to a bug in the reference software; the burden of proof is on {{producer}}.

I hereby agree not to interfer with the election process and to sign any blocks or confirmations necessary to facilitate transfer of control to the next set of producers as determined by the system contract.

I hereby acknolwedge that 2/3+ other producers elected producers may vote to disqualify {{producer}} in the event {{producer}} is unable to produce blocks or is unable to be reached.

If {{producer}} qualifies to receive compensation due to votes received, {{producer}} will provide a public endpoint allowing at least 100 peers to maintain synchronization with the blockchain and/or submit transactions to be included. {{producer}} shall maintain at least 1 validating node validating with full state and signature checking and shall report any objectively invalid blocks produced by the active block producers.

The community agrees to allow {{producer}} to authenticate peers as necessary to prevent abuse and denial of service attacks; however, {{producer}} agrees not to discriminate against non-abusive peers.

I agree to process transactions on a FIFO best-effort and to honestly bill transactions for measured execution time. 

I {{producer}} agree not to manipulate the contents of blocks in order to derive profit from:

1. the order in whcih transactions are included
2. the hash of the block that is produced 

I, {{producer}}, hereby agree to disclose and attest under penalty of purgery all ultimate beneficial owners of my company who own more than 1%.

I, {{producer}}, hereby agree not to promise benefits to special interest groups in exchange for votes.

I, {{producer}}, hereby agree to cooperate with other block producers.

I, {{producer}}, agree to maintain a website hosted at {{url}} which contains up-to-date information on all disclosures required by this contract.

I, {{producer}}, agree to set {{location}} such that {{producer}} is scheduled with minimal latency between my previous and next peer.

I, {{producer}}, agree to maintain time synchronization within 10 ms of global atomic clock time.

I, {{producer}}, agree not to produce blocks before my scheduled time unless I have received all blocks produced by the prior producer.

I, {{producer}}, agree not to publish blocks with timestamps more than 500ms in the future unless the prior block is more than 75% full by either CPU or network bandwidth metrics. 

I, {{producer}}, agree not to set the RAM supply to more RAM than my nodes contain and to resign if I am unable to provide the RAM approved by 2/3+ producers.  

## Constitution 

This agreement incorporates the current blockchain constitution:

{{constitution}}
