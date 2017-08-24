class Registrant {

  constructor( eth, eos = "", balance = 0 ){

    this.eth      = eth
    this.eos      = eos
    this.balance  = typeof balance == 'object' ? balance : new Balance()
    
    this.index    = null
    this.accepted = null
    this.error    = false
  
  }


  accept ( callback ) {

    this.accepted = true
    log("message", `[#${this.index}] accepted ${this.eth} => ${this.eos} => ${ this.balance.total.toFormat(4) }`)
  }


  reject () {
    
    this.accepted = false
    let msg = ""
    if(this.balance.exists('reclaimed')) 
      log("reject", `[#${this.index}] rejected ${this.eth} => ${this.eos} => ${this.balance.total.toFormat(4)} => ${this.error} ( ${this.balance.reclaimed.toFormat(4)} reclaimed EOS tokens moved back to Reclaimable )`)
    else 
      log("reject", `[#${this.index}] rejected ${this.eth} => ${this.eos} => ${this.balance.total.toFormat(4)} => ${this.error}`)

  }

  judgement() {
  
    return this.valid() ? this.accept() : this.reject()
  
  }


  set ( key, value ) {

    return (typeof this[`set_${key}`] === "function") ? this[`set_${key}`](value) : this

  }


  set_index ( index ) {

    this.index = index
    return this //for chaining

  }


  set_key ( eos_key ) {

    //Might be hex, try to convert it.
    if(eos_key.length == 106){                                    
      let eos_key_from_hex = web3.toAscii(eos_key) 
      if(eos_key_from_hex.startsWith('EOS') && eos_key_from_hex.length == 53) { 
        eos_key = eos_key_from_hex
      } 
    }

    //Might be user error
    else if(eos_key.startsWith('key')){                            
      let eos_key_mod = eos_key.substring(3) 
      if(eos_key_mod.startsWith('EOS') && eos_key_mod.length == 53) {
        eos_key = eos_key_mod
      } 
    }

    this.eos = eos_key

    return this //for chaining

  }


  // Reject bad keys and zero balances, elseif was fastest? :/
  valid() {

    //Reject 0 balances
    if( this.balance.total.lt(1) ) {
      this.error = 'balance_insufficient'
    }
    
    //Everything else
    else if(!this.eos.startsWith('EOS')) {
      
      //It's an empty key
      if(this.eos.length == 0) {
        this.error = 'key_is_empty'
      }
      
      //It may be an EOS private key
      else if(this.eos.startsWith('5')) { 
        this.error = 'key_is_private'
      }
      
      // It almost looks like an EOS key // #TODO ACTUALLY VALIDATE KEY?
      else if(this.eos.startsWith('EOS') && this.eos.length != 53) {
        this.error = 'key_is_malformed'
      }
      
      // ETH address
      else if(this.eos.startsWith('0x')) {
        this.error = 'key_is_eth'
      }
      
      //Reject everything else with label malformed
      else {
        this.error = 'key_is_junk'
      }
    }

    //Accept BTS and STM keys, assume advanced users and correct format
    if(this.eos.startsWith('BTS') || this.eos.startsWith('STM')) {
      this.error = false
    }

    return !this.error ? true : false

  }
}




class Balance {

  constructor(){
    this.wallet    = web3.toBigNumber(0)
    this.unclaimed = web3.toBigNumber(0)
    this.reclaimed = web3.toBigNumber(0)
    this.total     = web3.toBigNumber(0)
  }

  set( type, balance ){
    this[ type ] = balance
    return this //chaining
  }

  readable( type = 'total' ){
    this[ type ] = formatEOS( this[ type ] )
  }

  exists( type ){
    return (typeof this[ type ] !== "undefined" && this[type].gt( 0 ))
  }

  get( type ){
    return (typeof this[ type ] !== "undefined" && this[type].gt( 0 )) ? this[type] : false;
  }

  sum(){
    this.total = this.wallet.plus(this.unclaimed).plus(this.reclaimed)
  }
} 



class Transaction {

  constructor( eth, tx, type = "transfer", amount ) {
    this.eth     = eth
    this.eos     = null
    this.hash    = tx
    this.amount  = amount
    this.claimed = false
    this.type    = type
  } 

  claim( eth ) {
    return ( eth == this.eth ) 
      ? ( 
        this.claimed = true,
        log("success", `reclaimed ${this.eth} => ${this.eos} => ${this.amount.div(WAD).toFormat(4)} EOS <<< tx: https://etherscan.io/tx/${this.hash}`) 
      ) : log("error", `${eth} should't be claiming ${this.eth}'s transaction`)
  }

}

