# JavaScript Example

## Minimal Example

Install these dependencies:

```
npm install node-fetch zlib commander ws eosjs@20.0.0-beta3
```

This connects to the plugin, receives the protocol ABI, sends a `get_status_request_v0`, and displays the result:

```js
const { TextDecoder, TextEncoder } = require('util');
const { Serialize } = require('eosjs');
const commander = require('commander');
const fetch = require('node-fetch');
const zlib = require('zlib');
const WebSocket = require('ws');

class Connection {
    // Connect to the State-History Plugin
    constructor({ socketAddress }) {
        // Protocol ABI
        this.abi = null;

        // Types in the protocol ABI
        this.types = null;

        // Tables in the protocol ABI
        this.tables = new Map;

        // The socket
        this.ws = new WebSocket(socketAddress, { perMessageDeflate: false });
        this.ws.on('message', data => this.onMessage(data));
    }

    // Convert JSON to binary. type identifies one of the types in this.types.
    serialize(type, value) {
        const buffer = new Serialize.SerialBuffer({ textEncoder: new TextEncoder, textDecoder: new TextDecoder });
        Serialize.getType(this.types, type).serialize(buffer, value);
        return buffer.asUint8Array();
    }

    // Convert binary to JSON. type identifies one of the types in this.types.
    deserialize(type, array) {
        const buffer = new Serialize.SerialBuffer({ textEncoder: new TextEncoder, textDecoder: new TextDecoder, array });
        return Serialize.getType(this.types, type).deserialize(buffer, new Serialize.SerializerState({ bytesAsUint8Array: true }));
    }

    // Serialize a request and send it to the plugin
    send(request) {
        this.ws.send(this.serialize('request', request));
    }

    // Receive a message
    onMessage(data) {
        try {
            if (!this.abi) {
                // First message is the protocol ABI
                this.abi = JSON.parse(data);
                this.types = Serialize.getTypesFromAbi(Serialize.createInitialTypes(), this.abi);
                for (const table of this.abi.tables)
                    this.tables.set(table.name, table.type);

                // Send the first request
                this.send(['get_status_request_v0', {}]);
            } else {
                // Deserialize and dispatch a message
                const [type, response] = this.deserialize('result', data);
                this[type](response);
            }
        } catch (e) {
            console.log(e);
            process.exit(1);
        }
    }

    // Report status
    get_status_result_v0(response) {
        console.log('get_status_result_v0:');
        console.log(JSON.stringify(response, null, 4));
        process.exit(0);
    }
} // Connection

commander
    .option('-a, --socket-address [addr]', 'Socket address', 'ws://localhost:8080/')
    .option('-f, --first-block [num]', 'Socket address', 2)
    .parse(process.argv);

const connection = new Connection(commander);
```

