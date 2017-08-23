class Registrant {

  
  constructor( eth, eos = "", balance = 0 ){
    // let {unclaimed, wallet, recovered} = balances
    this.eth      = eth
    this.eos      = eos
    this.balance  = typeof balance == 'object' ? balance : new Balance()
  }

  set( key, value ){
    this[ key ] = value;
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

  update( type, balance ){
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
