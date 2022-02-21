const { TextDecoder, TextEncoder } = require('util');
const { Serialize } = require('eosjs');
const commander = require('commander');
const fetch = require('node-fetch');
const zlib = require('zlib');
const WebSocket = require('ws');

class Connection {
    // Connect to the State-History Plugin
    constructor(socketAddress, numRequests) {
        // Protocol ABI
        this.abi = null;

        // Types in the protocol ABI
        this.types = null;

        // Tables in the protocol ABI
        this.tables = new Map;

        this.startRequestArray = true;
        this.endRequestArray = false;
        this.numRequests = numRequests;
        this.blockNumChangeTime = null;
        this.createTime = Date.now();

        // The socket
        var status = { "status" : "construct" }
        status["time"] = Date.now()
        console.error('[');
        console.error(JSON.stringify(status, null, 4));
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
                this.abiTime = Date.now();
                var status = { "status" : "set_abi" }
                status["time"] = Date.now()
                console.error(', ');
                console.error(JSON.stringify(status, null, 4));
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
            var status = { "status" : "error" }
            status["time"] = Date.now()
            status["msg"] = e.toString();
            console.error(', ');
            console.error(JSON.stringify(status, null, 4));
            console.error(']');
            process.exit(1);
        }
    }

    // Report status
    get_status_result_v0(response) {
        if (this.startRequestArray) {
            this.startRequestArray = false;
            this.startTime = Date.now();
            console.log('[');
            response["start_time"] = this.startTime;
            this.blockNum = response["head"]["block_num"];
            this.initialBlockNum = this.blockNum;
            this.blockNumChangeTime = this.startTime;
            this.lastResponseTime = this.startTime;
            this.max = { "blockNum": 0, "time": 0.0 };
        } else {
            console.log(',' + "\n");
        }
        console.log('{ \"get_status_result_v0\":');
        response["resp_num"] = this.numRequests;
        var currentTime = new Date();
        var currentTimeSec = currentTime.getTime()
        response["resp_time"] = (currentTimeSec - this.startTime) / 1000;
        var timeSinceBlockNumChange = (currentTimeSec - this.blockNumChangeTime) / 1000;
        response["time_since_block_num_change"] = timeSinceBlockNumChange;
        var blockNum = response["head"]["block_num"]
        if (blockNum != this.blockNum) {
            this.blockNum = response["head"]["block_num"];
            if (this.max["time"] < timeSinceBlockNumChange) {
                this.max["time"] = timeSinceBlockNumChange;
                this.max["blockNum"] = blockNum;
            }
            this.blockNumChangeTime = currentTimeSec;
        }
        response["time_since_last_block"] = (currentTimeSec - this.lastResponseTime) / 1000;
        response["current_time"] = currentTime.toISOString()
        this.lastResponseTime = currentTimeSec;
        console.log(JSON.stringify(response, null, 4));
        console.log('}');
        if (this.numRequests < 0) { // request forever
            this.send(['get_status_request_v0', {}]);
        } else if (--this.numRequests > 0 ) {
            this.send(['get_status_request_v0', {}]);
        } else {
            console.log(']');
            var status = { "status" : "done" }
            console.error(' ,');
            status["time"] = Date.now()
            if (this.max["time"] < timeSinceBlockNumChange) {
                this.max["time"] = timeSinceBlockNumChange;
                this.max["blockNum"] = blockNum;
            }
            status["max_time"] = {"block_num": this.max["blockNum"], "time_delta": this.max["time"] };
            status["first_block_num"] = this.initialBlockNum;
            status["last_block_num"] = blockNum;
            status["abi_rcv_delta_time"] = (this.abiTime - this.createTime)/1000;
            status["req_rcv_delta_time"] = (this.startTime - this.createTime)/1000;
            status["done_delta_time"] = (currentTimeSec - this.createTime)/1000;

            console.error(JSON.stringify(status, null, 4));
            console.error(']');
            process.exit(0);
        }
    }
} // Connection

commander
    .option('-a, --socket-address [addr]', 'Socket address', 'ws://localhost:8080/')
    .option('-f, --first-block [num]', 'Socket address', 2)
    .option('-n, --num-requests [num]', 'number of requests to make', 1)
    .parse(process.argv);

const connection = new Connection(commander.socketAddress, commander.numRequests);

