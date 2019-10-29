<h1 class="clause">UserAgreement</h1>

User agreement for the chain can go here.

<h1 class="clause">BlockProducerAgreement</h1>

I, {{producer}}, hereby nominate myself for consideration as an elected block producer.

If {{producer}} is selected to produce blocks by the system contract, I will sign blocks with my registered block signing keys and I hereby attest that I will keep these keys secret and secure.

If {{producer}} is unable to perform obligations under this contract I will resign my position using the unregprod action.

I acknowledge that a block is 'objectively valid' if it conforms to the deterministic blockchain rules in force at the time of its creation, and is 'objectively invalid' if it fails to conform to those rules.

{{producer}} hereby agrees to only use my registered block signing keys to sign messages under the following scenarios:

* proposing an objectively valid block at the time appointed by the block scheduling algorithm;
* pre-confirming a block produced by another producer in the schedule when I find said block objectively valid;
* and, confirming a block for which {{producer}} has received pre-confirmation messages from more than two-thirds of the active block producers.

I hereby accept liability for any and all provable damages that result from my:

* signing two different block proposals with the same timestamp;
* signing two different block proposals with the same block number;
* signing any block proposal which builds off of an objectively invalid block;
* signing a pre-confirmation for an objectively invalid block;
* or, signing a confirmation for a block for which I do not possess pre-confirmation messages from more than two-thirds of the active block producers.

I hereby agree that double-signing for a timestamp or block number in concert with two or more other block producers shall automatically be deemed malicious and cause {{producer}} to be subject to:

* a fine equal to the past year of compensation received,
* immediate disqualification from being a producer,
* and/or other damages.

An exception may be made if {{producer}} can demonstrate that the double-signing occurred due to a bug in the reference software; the burden of proof is on {{producer}}.

I hereby agree not to interfere with the producer election process. I agree to process all producer election transactions that occur in blocks I create, to sign all objectively valid blocks I create that contain election transactions, and to sign all pre-confirmations and confirmations necessary to facilitate transfer of control to the next set of producers as determined by the system contract.

I hereby acknowledge that more than two-thirds of the active block producers may vote to disqualify {{producer}} in the event {{producer}} is unable to produce blocks or is unable to be reached, according to criteria agreed to among block producers.

If {{producer}} qualifies for and chooses to collect compensation due to votes received, {{producer}} will provide a public endpoint allowing at least 100 peers to maintain synchronization with the blockchain and/or submit transactions to be included. {{producer}} shall maintain at least one validating node with full state and signature checking and shall report any objectively invalid blocks produced by the active block producers. Reporting shall be via a method to be agreed to among block producers, said method and reports to be made public.

The community agrees to allow {{producer}} to authenticate peers as necessary to prevent abuse and denial of service attacks; however, {{producer}} agrees not to discriminate against non-abusive peers.

I agree to process transactions on a FIFO (first in, first out) best-effort basis and to honestly bill transactions for measured execution time.

I {{producer}} agree not to manipulate the contents of blocks in order to derive profit from: the order in which transactions are included, or the hash of the block that is produced.

I, {{producer}}, hereby agree to disclose and attest under penalty of perjury all ultimate beneficial owners of my business entity who own more than 10% and all direct shareholders.

I, {{producer}}, hereby agree to cooperate with other block producers to carry out our respective and mutual obligations under this agreement, including but not limited to maintaining network stability and a valid blockchain.

I, {{producer}}, agree to maintain a website hosted at {{url}} which contains up-to-date information on all disclosures required by this contract.

I, {{producer}}, agree to set the location value of {{location}} such that {{producer}} is scheduled with minimal latency between my previous and next peer.

I, {{producer}}, agree to maintain time synchronization within 10 ms of global atomic clock time, using a method agreed to among block producers.

I, {{producer}}, agree not to produce blocks before my scheduled time unless I have received all blocks produced by the prior block producer.

I, {{producer}}, agree not to publish blocks with timestamps more than 500ms in the future unless the prior block is more than 75% full by either NET or CPU bandwidth metrics.

I, {{producer}}, agree not to set the RAM supply to more RAM than my nodes contain and to resign if I am unable to provide the RAM approved by more than two-thirds of active block producers, as shown in the system parameters.
