# Chain API v2 Plugin

This plugin provides new APIs to send a transaction to EOS blockchain and then wait for its finalization.

Here are the list of new APIs in this plugin.

* Push a transaction to be tracked, the server would response when at least one producer accepts it.
    * URL: `/v2/chain/push_transaction`
    * Method: `POST`
    * Body Params: same with `/v1/chain/push_transaction`
    * Success Response: same with `/v1/chain/push_transaction`
    * Error Response: same with `/v1/chain/push_transaction`
* Send a transaction to be tracked, the server would response when at least one producer accepts it.
    * URL: `/v2/chain/send_transaction`
    * Method: `POST`
    * Body Params: same with `/v1/chain/send_transaction`
    * Success Response: same with `/v1/chain/send_transaction`
    * Error Response: same with `/v1/chain/send_transaction`
* Wait for a tracked transaction to be finalized.
    * URL: `/v2/chain/wait_transaction`
    * Method: `POST`
    * Body Params: `{"transaction_id": "{the tracked transaction id}" }`
    * Success Response: `201 Created`
    * Error Response:
        * `403 Forbidden`: there is already a pending waiting request on the transaction.
        * `404 Not Found`:
            * the transaction is not sent through `/v2/chain/push_transaction` or `/v2/chain/send_transaction`, or
            * the expiration time of the transaction is earlier than latest irreversible block received by the server.
        * `504 Gateway Timeout`: the transaction expires.
