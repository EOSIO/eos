const unhex      = data                       => data.replace(/^0x/, "")
const hex        = data                       => `0x${unhex(data)}`
const bytes4     = data                       => hex(unhex(data).slice(0, 8))
const sighash    = sig                        => bytes4(web3.sha3(sig))
const parts      = (data, n)                  => data.match(new RegExp(`.{${n}}`, "g")) || []
const hexparts   = (data, n)                  => parts(unhex(data), n).map(hex)
const words32    = data                       => hexparts(data, 64)
const bytes      = data                       => hexparts(data, 2)
const hexcat     = (...data)                  => hex(data.map(unhex).join(""))
const word       = data                       => hex(unhex(data).padStart(64, "0"))
const calldata   = (sig, ...words)            => hexcat(sig, ...words.map(word))
const toAsync    = promise                    => $ => promise.then(x => $(null, x), $)
const byId       = id                         => document.getElementById(id)
const formatWad  = wad                        => String(wad).replace(/\..*/, "")
const formatEOS  = wad                        => wad ? wad.toFormat(4).replace(/,/g , "") : "0"
const formatETH  = wad                        => wad ? wad.toFormat(2).replace(/,/g , "") : "0"
const getValue   = id                         => byId(id).value
const show       = id                         => byId(id).classList.remove("hidden")
const hide       = id                         => byId(id).classList.add("hidden")
const iota       = n                          => repeat("x", n).split("").map((_, i) => i)
const repeat     = (x, n)                     => new Array(n + 1).join("x")
const getTime    = ()                         => new Date().getTime() / 1000

// Getters

// Transactions
const get_transactions_reclaimable = ()       => transactions.filter( tx => tx.type == 'transfer' && tx.amount.gt(0) && (!tx.claimed ||  (tx.claimed && get_registrants_rejected().filter( registrant => registrant.eth == tx.eth ).length) ) )
const get_transactions_reclaimed = ()         => transactions.filter( tx => tx.claimed && get_registrants_accepted().filter( registrant => registrant.eth == tx.eth ).length )

// Registrant Collection
const get_registrants_accepted = ()           => registrants.filter( registrant => registrant.accepted ) 
const get_registrants_rejected = ()           => registrants.filter( registrant => registrant.accepted === false ) 
const get_registrants_unprocessed = ()        => registrants.filter( registrant => registrant.accepted === null )

// Registrant
const get_registrant_reclaimed = ( eth )      => transactions.filter( tx => tx.type == 'transfer' && tx.claimed && eth == tx.eth ) 
const get_registrant_reclaimable = ( eth )    => transactions.filter( tx => tx.type == 'transfer' && !tx.claimed && eth == tx.eth && tx.amount.gt( 0 ) )

// Web3
const node_syncing = ( callback = () => {} )  => setTimeout( () => { is_synced() ? callback() : node_syncing( callback ) }, 3000)
const disconnected = ( callback = () => {} )  => setTimeout( () => { is_listening() ? callback() : disconnected( callback ) }, 3000)
const is_synced = ()                          => !web3.eth.syncing ? true : (web3.eth.syncing.currentBlock > SS_LAST_BLOCK ? true : false)

// Crowdsale Helpers
const period_for = timestamp                  => timestamp < CS_START_TIME ? 0 : Math.min(Math.floor((timestamp - CS_START_TIME) / 23 / 60 / 60) + 1, 350);
const period_eth_balance = ()                 => contract.$utilities.dailyTotals().map(web3.toBigNumber)
const get_last_block_time = ()                => web3.eth.getBlock(SS_LAST_BLOCK).timestamp
const get_supply = ( periods = 350 )          => web3.toBigNumber( CS_CREATE_PER_PERIOD ).times(periods).plus( web3.toBigNumber(CS_CREATE_FIRST_PERIOD) ).plus( web3.toBigNumber(SS_BLOCK_ONE_DIST)  )
const to_percent = ( result )                 => Math.floor( result * 100 )

// Check if everything is working
const is_listening = () => {
  let listening = true
  try { web3.isConnected() }
  catch(e) { listening = false }
  return listening
}

//logger helper
const log = ( type, msg = "" ) => {
  if(msg.length && OUTPUT_LOGGING) output.logs.push(msg)
  if(console && console.log && CONSOLE_LOGGING && typeof logger[type] !== 'undefined') logger[type](msg)
}

const logger = {};
logger.success        = (msg) => console.log(`%c ${msg}`, 'font-weight:bold; color:green; background:#DEF6E6; ')
logger.error          = (msg) => console.log(`%c ${msg}`, 'font-weight:bold; color:red;')  
logger.bold           = (msg) => console.log(`%c ${msg}`, 'font-weight:bold;') 
logger.info           = (msg) => console.log(`%c ${msg}`,'font-style:italic; color:#888;' )
logger.block_error    = (msg) => console.log(`%c ${msg}`,'width:100%; background-color:red; color:#FFF; font-weight:bold; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:9pt' )
logger.block_success  = (msg) => console.log(`%c ${msg}`,'width:100%; background-color:green; color:#FFF; font-weight:bold; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:9pt' )
logger.debug          = (msg) => console.log(`%c ${msg}`, log_style_debug)  
logger.message        = (msg) => console.log(`%c ${msg}`, 'color:#666')  
logger.reject         = msg   => console.log(`%c ${msg}`, 'color:#b0b0b0; text-decoration:line-through; font-style:italic; ')
logger.block          = (msg) => console.log(`%c ${msg}`,'width:100%; background-color:black; color:#FFF; font-weight:bold; line-height:30px; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:10pt' )
logger.largest_text   = (msg) => console.log(`%c ${msg}`,'line-height:45px; font-size:33pt; color:#dedede; font-weight:100; display:inline-block ' )
logger.large_text     = (msg) => console.log(`%c ${msg}`,'line-height:45px; font-size:14pt; color:#c0c0c0; font-weight:100; display:inline-block ' )
logger.group          = (msg) => console.group(msg)
logger.groupCollapsed = (msg) => console.groupCollapsed(msg)
logger.groupEnd       = ()    => console.groupEnd()     
