/**
 * The following stress test will send bunch of transfer transactions
 * - transfer from inita to initb
 * - transfer from initb to initc
 * - transfer from initc to initd
 * - .....
 * - transfer from initu to inita
 * equally distributed to each node in the testnet
 */

const rp = require("request-promise");
const _ = require("lodash");
const assert = require("assert");
const config = require("./config.json");
const fs = require("fs");

// Check completeness of config
assert(config.hasOwnProperty("testnetUris"), "missing testnet uris");
assert(config["testnetUris"].length > 0, "testnet uris > 0");
assert(config.hasOwnProperty("serializeTrxNodeUri"), "missing serialize trx node uri");
assert(config.hasOwnProperty("minNumOfTrxToSend"), "missing min num of trx to send");
assert(config["minNumOfTrxToSend"] > 0, "min num of trx per batch > 0");
assert(config.hasOwnProperty("numOfTrxPerBatch"), "missing num of trx per batch");
assert(config["numOfTrxPerBatch"] <= 1000 && config["numOfTrxPerBatch"] > 0, "0 < num of trx per batch <= 1000");
assert(config.hasOwnProperty("maxSimulHttpReqPerNode"), "missing max simultaneous http request per node");

const infoPath = "/v1/chain/get_info";
const blockPath = "/v1/chain/get_block";
const listKeysPath = "/v1/wallet/list_keys";
const abiJsonToBinPath = "/v1/chain/abi_json_to_bin";
const signTrxPath = "/v1/wallet/sign_transaction";
const pushTrxsPath = "/v1/chain/push_transactions";

const publicKeys = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"];
const accounts = [
    "inita",
    "initb",
    "initc",
    "initd",
    "inite",
    "initf",
    "initg",
    "inith",
    "initi",
    "initj",
    "initk",
    "initl",
    "initm",
    "initn",
    "inito",
    "initp",
    "initq",
    "initr",
    "inits",
    "initt",
    "initu"
];

// Get request option for HTTP request
const getRequestOptions = (nodeUri, path, param) => {
    return {
        method: "POST",
        uri: nodeUri + path,
        body: param,
        json: true 
    };
}

/**
 * Get param for /v1/chain/abi_json_to_bin
 */
let nonce = 0;
const getAbiJsonToBinParam = (from, to) => {
    const param = {
        "code":"eos",
        "action":"transfer",
        "args": {
            "from":from,
            "to":to,
            "amount":100,
            "memo":String(nonce)
        }
    }
    nonce++;
    return param;
}

/**
 * Get param for /v1/wallet/sign_transaction
 */
const getSignTrxParam = (serializedTrx) => {
    return [
        serializedTrx,
        publicKeys,
        ""
    ];
}

/**
 * Get keys stored inside the wallet
 */
const getWalletKeys = () => {
    const listKeysOption = getRequestOptions(config.walletUri, listKeysPath);
    return rp(listKeysOption);
}

/**
 * Check if the wallet has the required key
 * @param walletKeys keys
 */
const hasValidKey = (walletKeys) => {
    let valid = false;
    _.forEach(walletKeys, (keyPair) => {
        if (keyPair[0] === publicKeys[0]) valid = true;
    })
    return valid;
}

/**
 * Get Reference Block Information
 */
const getRefBlockInfo = () => {
    const nodeUri = config.testnetUris[0];

    const infoOption = getRequestOptions(nodeUri, infoPath);
    const request = rp(infoOption).then(info => {
        const blockParam = {
            block_num_or_id: info.head_block_num
        };
        const blockOption = getRequestOptions(nodeUri, blockPath, blockParam);
        return rp(blockOption);
    });

    return request;
}

/**
 * Generate a collection of serialized transfer transactions
 * @param  refBlockInfo reference block info
 * @returns serialized transactions
 */
const generateSerializedTrxs = (refBlockInfo) => {
    let serializedTrxs = [];
    
    // Create single serialized transaction
    const createSerializedTrx = (refBlockInfo, from, to) => {
        const abiJsonToBinParam = getAbiJsonToBinParam(from, to);
        const abiJsonToBinOptions = getRequestOptions(config.serializeTrxNodeUri, abiJsonToBinPath, abiJsonToBinParam);
        return rp(abiJsonToBinOptions).then(result => {
            const trxExpTime = new Date( new Date().getTime() + 1000*60*30);
            const trxExpTimeStr = trxExpTime.toISOString().slice(0,-5);
    
            const serializedTrx = {
                "ref_block_num":refBlockInfo.block_num,
                "ref_block_prefix":refBlockInfo.ref_block_prefix,
                "expiration":trxExpTimeStr,
                "scope":_.sortBy([from,to]),
                "messages":[{
                    "code":"eos",
                    "type":"transfer",
                    "authorization":[
                        {
                            "account":from,
                            "permission":"active"
                        }
                    ],"data":result.binargs
                }
                ],
                "signatures":[],
                "authorizations":[]
            }
            return serializedTrx;
        }).catch(error => {
            console.error("Caught an error!", error);
            process.exit();
        })
    }
    
    // Create one round promise
    // One round is defined to be set of requests that is limited by number of max parallel request at a time
    const oneRound = (counter) => {
        const serializedTrxPromises = [];
        
        // One cycle is defined to be a set of transfer from inita->initb->...->initu->inita
        const oneCycle = () => {
            for (let j = 0; j < accounts.length; j++) {
                const from = accounts[j];
                const to =  (j + 1) < accounts.length ? accounts[j+1] : accounts[0];
                serializedTrxPromises.push(createSerializedTrx(refBlockInfo, from, to));
            }
        }

        const numOfCycleInOneRound = Math.max(1, Math.floor(config.maxSimulHttpReqPerNode / accounts.length));
        for (let i = 0; i < numOfCycleInOneRound; i++) {
           oneCycle();
        }

        return Promise.all(serializedTrxPromises).then(result => {
            console.log("Get serialized trxs round ", counter);
            serializedTrxs = serializedTrxs.concat(result);
            return serializedTrxs;
        });
    }

    const numOfTrxPerRound = Math.max(accounts.length, Math.floor(config.maxSimulHttpReqPerNode / accounts.length) * accounts.length);
    const numOfRound = Math.ceil(config.minNumOfTrxToSend / numOfTrxPerRound);

    // Chain the promise, i.e. requests in series
    let promiseChain = Promise.resolve();
    for (let i = 0; i < numOfRound; i++) {
        promiseChain = promiseChain.then(() => {
            return oneRound(i);
        });
    }
    return promiseChain;
}

/**
 * Sign serialized transactions
 * @param serializedTrxs transactions to be signed
 * @returns signed transactions
 */
const getSignedTrxs = (serializedTrxs) => {
    let signedTrxs = [];

    // Create one round promise
    // One round is defined to be set of requests that is limited by number of max parallel request at a time
    const oneRound = (messages, counter) => {
        const signTrxPromises = [];
        for (let i = 0; i < messages.length; i++) {
            const serializedTrx = messages[i];
            const signTrxParam = getSignTrxParam(serializedTrx);
            const signTrxOption = getRequestOptions(config.walletUri, signTrxPath, signTrxParam);
            const signTrxPromise = rp(signTrxOption);
            signTrxPromises.push(signTrxPromise);
        } 
        return Promise.all(signTrxPromises).then(result => {
            console.log("Get signed trxs round ", counter);
            signedTrxs = signedTrxs.concat(result);
           
            return signedTrxs;
        });
    }

    // Chain the promise, i.e. requests in series
    let promiseChain = Promise.resolve();
    let counter = 0;
    while (serializedTrxs.length > 0) {
        const spliced = serializedTrxs.splice(0, config.maxSimulHttpReqPerNode);
        promiseChain = promiseChain.then(() => {
            return oneRound(spliced, counter++);
        });
    }
   
    return promiseChain;
}

/**
 * Push signed transactions
 * @param signedTrxs signed transactions to be pushed
 */
const pushTrxs = (signedTrxs) => {

    const resultFileName = "stress_test_result_"+new Date().getTime()+".txt";
    // Create one round promise
    // One round is defined to be set of requests that is limited by number of max parallel request at a time
    const oneRound = (signedTrxs, counter) => {
        const pushTrxsPromises = [];

        // Divide the transactions into batch
        let iteration = 0;
        while (signedTrxs.length > 0) {
            // Get node uri to send the trxs to 
            const nodeUri = config.testnetUris[iteration % config.testnetUris.length];
            console.log("Sending " + Math.min(config.numOfTrxPerBatch, signedTrxs.length) + " transactions to node " + nodeUri);

            const pushTrxsParam = signedTrxs.splice(0, config.numOfTrxPerBatch);
            const pushTrxsOption = getRequestOptions(nodeUri, pushTrxsPath, pushTrxsParam);
            const pushTrxsPromise = rp(pushTrxsOption);
            pushTrxsPromises.push(pushTrxsPromise);
            iteration++;
        }
       
        return Promise.all(pushTrxsPromises).then(result => {
            console.log("Push trx round ", counter);

            // Log first and last transaction per round for reference
            const firstTx = result[0][0];
            const lastTx = result[result.length-1][result[result.length-1].length-1];
            console.log("First trx of round " + counter + " ...");
            console.log(JSON.stringify(firstTx));
            console.log("Last trx of round " + counter + " ...");
            console.log(JSON.stringify(lastTx));

            // Append result
            fs.appendFile(resultFileName, JSON.stringify(result, null, 2), (error) => {
                if (error) {
                    console.error("Fail to write result to a file!");
                } else {
                    console.log("Result written to a file");
                }
            });
        });
    }

    const maxSimulOutgoingHttpReq = config.maxSimulHttpReqPerNode * config.testnetUris.length;
    const numOfTrxsPerRound = Math.min(maxSimulOutgoingHttpReq * config.numOfTrxPerBatch, signedTrxs.length);
    const numOfParallelReq = Math.ceil(numOfTrxsPerRound / config.numOfTrxPerBatch);
    const numOfIteration =  Math.ceil(signedTrxs.length / numOfTrxsPerRound);

    console.log("Number of transactions per batch: " + config.numOfTrxPerBatch);
    console.log("Number of transactions per round: " + numOfTrxsPerRound);
    console.log("Number of parallel requests per node: " + numOfParallelReq);
    console.log("Number of push transactions iterations: " + numOfIteration);

    // Chain the promise, i.e. requests in series
    let promiseChain = Promise.resolve();
    let counter = 0;
    while(signedTrxs.length > 0) {
        const spliced = signedTrxs.splice(0, numOfTrxsPerRound);
        promiseChain = promiseChain.then(() => {
            return oneRound(spliced, counter++);
        });
    }
    return promiseChain;
}

// Start!
console.log("Minimum number of transactions to send: " + config.minNumOfTrxToSend);
console.log("Checking wallet validity...");
getWalletKeys().then((walletKeys) => {
    // Assert that the wallet has valid keys
    assert(hasValidKey(walletKeys), "please import inita key to your wallet");
    console.log("Get ref block info...");
    return getRefBlockInfo();
}).then((refBlockInfo) => {
    // Get serialized transactions
    console.log("Get serialized trxs...");
    return generateSerializedTrxs(refBlockInfo);
}).then((serializedTrxs) => {
    console.log("Number of trxs generated: " + serializedTrxs.length);
    // Get signed transactions
    console.log("Get signed trxs...");
    return getSignedTrxs(serializedTrxs);
}).then((signedTrxs) => {
    // Push signed transactions
    console.log("Push signed trxs...");
    return pushTrxs(signedTrxs);
}).then((result) => {
    console.log("Finish!");
}).catch((error) => {
    // Print error
    console.error("Caught an error!");
    console.error(error);
    console.error("Finish with error!");
});
    


