# State-History Plugin

This plugin tracks chain state and transaction history and provides this data to external processes through a websocket.

It stores:
* gzipped transaction traces. These include the traces for normal transactions and deferred transactions. Inline actions are present.
* gzipped state deltas. These include changes to chain state plus contract state.

The websocket streams data to external processes. It automatically catches clients up to head, then notifies them as the chain progresses or switches forks.

# Temporary TOC

* [State-History Plugin](state-history-plugin.md)
   * [Setup](setup.md)
   * [Protocol](protocol.md)
   * [JS Example](js-example.md)
