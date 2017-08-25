//Change if needed
const NODE                      = "http://127.0.0.1:8545"                       //Default Host

//testnet || mainnet
const SNAPSHOT_ENV              = "testnet"

//User Options
let   CONSOLE_LOGGING           = true    // Push activity to developer console, WARNING: unstable in Firefox and Safari
let   OUTPUT_LOGGING            = false   // Generate an output log file (±6MB), WARNING: degrades browser performance significantly.
let   VERBOSE_REGISTRY_LOGS     = false   // Generate a log entry based on LogRegistry events, WARNING: degrades performance
let   UI_SHOW_STATUS_EVERY      = 150     // Shows some status information in console every X logs, useful for debugging.

// Needs to be set so all snapshots generated are the somewhat the same 
// Caveat: testnet snapshots will rarely be the same because wallet balances can still change, but setting last block *will* provide consistent registry totals.
const SS_FIRST_BLOCK            = 3904416       //Block EOS Contract was deployed at
const SS_LAST_BLOCK             = 4167293       //Last block to honor ethereum transactions [NOT FINAL]

//Test Case: Quick, Has orphaned TX, Has a Reclaimed Registrant and has two types of rejects.
// const SS_FIRST_BLOCK            = 4146970    //Block EOS Contract was deployed at
// const SS_LAST_BLOCK             = 4148293    //Last block to honor ethereum transactions

//For Web3 Init
const CROWDSALE_ADDRESS         = "0xd0a6E6C54DbC68Db5db3A091B171A77407Ff7ccf"  //for Mapping
const TOKEN_ADDRESS             = "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0"  //for balanceOf()
const UTILITY_ADDRESS           = "0x860fd485f533b0348e413e65151f7ee993f93c02"  //Useful functions

//Crowdsale Figures
const WAD                       = 1000000000000000000
const SS_BLOCK_ONE_DIST         = 100000000 * WAD
let   SS_PERIOD_ETH
let   SS_LAST_BLOCK_TIME    

const CS_NUMBER_OF_PERIODS      = 350
const CS_CREATE_FIRST_PERIOD    = 200000000 * WAD
const CS_CREATE_PER_PERIOD      = 2000000 * WAD
const CS_START_TIME             = 1498914000