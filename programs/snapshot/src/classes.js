class Registrant {

  constructor( eth, eos = "", balance = 0 ){
    // let {unclaimed, wallet, recovered} = balances
    this.eth      = eth
    this.eos      = eos
    this.balance  = typeof balance == 'object' ? balance : new Balance()
    this.accepted = null
    this.error    = false
    this.index    = null
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

  test() {
    return this.valid() ? this.accept() : this.reject()
  }

    // Reject bad keys and zero balances, elseif was fastest? :/
  valid() {

    //Reject 0 balances
    if( formatEOS( this.balance.total ) === "0.0000" ) {
      this.error = 'balance_is_zero'
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

    return !this.error ? true : false;

  }

  is_accepted () {
    return this.accepted === true 
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
      ? this.claimed = true
      : log("error", `${eth} should't be claiming ${this.eth}'s transaction`)
  }

}

class RejectedRegistrant extends Registrant {
  constructor( error, registrant ) {
    
    const { eth, eos, balance } = registrant
    
    super( registrant.eth, registrant.eos, registrant.balance )
    
    this.error             = error
  
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

  add( type, balance ){
    this['type'].plus( balance )
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
