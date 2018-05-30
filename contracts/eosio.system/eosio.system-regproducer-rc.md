# eosio.system regproducer

This Ricardian contract for the system action *regproducer* is legally binding and can be used in the event of a dispute. 

## regproducer
    ( const account_name producer
    , const public_key& producer_key
    , const std::string& url
    , uint16_t location );

_Intent: create a producer-config and producer-info object for 'producer'_

As an authorized party I {{ signer }} wish to register a producer candidate named {{ account_name }} and identified with {{ producer_key }} located at {{ url }} and located at {{ location }}.

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
