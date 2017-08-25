//For debugging and repurposed for gui
const calculator = function(){

  let debug = {}

  debug.tests = {}

  debug.registry = {         
      total:                                0 // Total Users found in LogRegister
      ,accepted:                            0 // Registrants with valid keys and balance
      ,rejected:                            0 // Registrants with invalid keys or zero balances
    }

  debug.reclaimable = {
      total:                                0
    }

  debug.rejected = {
      balance_is_zero:                      0 // Registrants with no balance
      ,key_is_empty:                        0 // key===""
      ,key_is_eth:                          0 // Ethereum key 0x...
      ,key_is_malformed:                    0 // Starts with EOS... but isn't a key
      ,key_is_junk:                         0 // Garbage
      ,total:                               0
      ,$key_is_empty:                       web3.toBigNumber(0) 
      ,$key_is_eth:                         web3.toBigNumber(0) 
      ,$key_is_malformed:                   web3.toBigNumber(0) 
      ,$key_is_junk:                        web3.toBigNumber(0) 
      ,$total:                              web3.toBigNumber(0) // ^^^^^^ sum
    }

  debug.distribution = {
      $total_supply:                        web3.toBigNumber(0) // 2M(periods-1)+200M (not static for testnet snapshot)
      ,$snapshot_supply:                    web3.toBigNumber(0) // 2M(periods-1)+200M (not static for testnet snapshot)
      ,$balance_wallets:                    web3.toBigNumber(0) // sum(tokens in registrant walthis.balances)
      ,$balance_unclaimed:                  web3.toBigNumber(0) // sum(tokens still locked in contract)
      ,$balance_reclaimable:                web3.toBigNumber(0) // crowdsale contract EOS token balance + token contract EOS token balance
      ,$balance_reclaimed:                  web3.toBigNumber(0) // crowdsale contract EOS token balance + token contract EOS token balance
      ,$balance_rejected:                   web3.toBigNumber(0) // sum(tokens in registrant walthis.balances)
      ,$balances_found:                     web3.toBigNumber(0) // sum(wallet, unclaimed, reclaimable)
      ,$balances_missing:                   web3.toBigNumber(0) // (tokens_supply-balances_found)/totens_total
  }

  debug.rates = {
      registrant_acceptance:                null // registrants.accepted / registrants.total
      ,balance_wallets:                     null // distribution.balance_wallets / distribution.balances_found
      ,balance_unclaimed:                   null // 
      ,balance_reclaimable:                 null //
      ,balance_reclaimed:                   null //
      ,balances_found:                      null // sum(wallet, unclaimed, reclaimable)
      ,balances_missing:                    null // (tokens_supply-balances_found)/totens_total
  }

  debug.update = function(){
      //registrants
      this.registry.total                   = registrants.length
      this.registry.accepted                = get_registrants_accepted().length
      this.registry.rejected                = get_registrants_rejected().length

      //Rejection Token Value
      this.rejected.$key_is_empty           = this.sum_rejection( "key_is_empty" )
      this.rejected.$key_is_eth             = this.sum_rejection( "key_is_eth" ) 
      this.rejected.$key_is_malformed       = this.sum_rejection( "key_is_malformed" )
      this.rejected.$key_is_junk            = this.sum_rejection( "key_is_junk" )
      this.rejected.$total                  = get_registrants_rejected().filter( reject => { return reject.rejected != "balance_is_zero" } ).map( reject => { return reject.balance.total }).reduce( ( acc, balance )  => balance.plus(acc), web3.toBigNumber(0)  )
      
      //Rejection Occurences
      this.rejected.balance_is_zero         = this.sum_rejection_occurences( "balance_is_zero" )
      this.rejected.key_is_empty            = this.sum_rejection_occurences( "key_is_empty" )
      this.rejected.key_is_eth              = this.sum_rejection_occurences( "key_is_eth" ) 
      this.rejected.key_is_malformed        = this.sum_rejection_occurences( "key_is_malformed" )
      this.rejected.key_is_junk             = this.sum_rejection_occurences( "key_is_junk" )

      //Distribution 1
      
      this.distribution.$total_supply       = web3.toBigNumber( get_supply() ).div(WAD)
      this.distribution.$snapshot_supply    = web3.toBigNumber( get_supply( period_for( SS_LAST_BLOCK_TIME) ) ).div(WAD)
      
      this.distribution.$balances_found     = this.sum_balance_all()
      this.distribution.$balance_wallets    = this.sum_balance('wallet')
      this.distribution.$balance_unclaimed  = this.sum_balance('unclaimed')
      this.distribution.$balance_reclaimed  = this.sum_balance('reclaimed')

      //Distribution
      this.distribution.$balances_missing    = this.distribution.$snapshot_supply.minus(this.distribution.$balances_found)
      this.distribution.$balance_reclaimable = get_transactions_reclaimable().reduce( (sum, tx) => { return tx.amount.div(WAD).plus( sum ) }, web3.toBigNumber(0) )
      this.reclaimable.total                 = get_transactions_reclaimable().length

      //Rates
      this.rates.percent_complete           = to_percent( ( this.registry.accepted + this.registry.rejected ) / this.registry.total )
      this.rates.balances_found             = to_percent( this.distribution.$balances_found.div(this.distribution.$snapshot_supply ).toFixed(2) )
      this.rates.registrant_acceptance      = to_percent( this.registry.accepted / ( this.registry.accepted + this.registry.rejected ) ) 
      this.rates.balance_wallets            = to_percent( this.distribution.$balance_wallets.div(this.distribution.$balances_found ).toFixed(2) )
      this.rates.balance_unclaimed          = to_percent( this.distribution.$balance_unclaimed.div(this.distribution.$balances_found ).toFixed(2) )
      this.rates.balance_reclaimed          = to_percent( this.distribution.$balance_reclaimed.div(this.distribution.$balances_found ).toFixed(2) )
      this.rates.balance_reclaimable        = to_percent( this.distribution.$balance_reclaimable.div(this.distribution.$snapshot_supply ).toFixed(2) )
      this.rates.balance_reclaimed_success  = to_percent( this.distribution.$balance_reclaimed.div( this.distribution.$balance_reclaimable.plus(this.distribution.$balance_reclaimed) ).toFixed(2) )
      this.rates.balance_reclaimable_total  = to_percent( this.distribution.$balance_reclaimable.div(this.distribution.$total_supply ).toFixed(2) )

      //Tests
      this.tests.balances                   = this.distribution.$balance_wallets.plus(this.distribution.$balance_unclaimed).plus(this.distribution.$balance_reclaimed).eq( this.distribution.$balances_found ) ? "PASS" : `FAIL ${sum.toFixed(4)} != ${ this.distribution.$balances_found.toFixed(4) }`
      this.tests.precision_loss             = this.distribution.$balance_wallets.plus(this.distribution.$balance_unclaimed).plus(this.distribution.$balance_reclaimed).minus(this.distribution.$balances_found)
      
      let _registrants                      = new Set()
      this.tests.unique                     = !registrants.some( registrant => _registrants.size === _registrants.add(registrant.eth).size ) ? "PASS" : "FAIL"


  }

  debug.find = function(group, key){
      return (typeof this[group] === 'object' && typeof this[type][key] !== 'undefined') ? this[type][key] : web3.toBigNumber(0)
  }

  debug.set = function(group, key, value){
     (typeof this[group] === 'object' && typeof this[type][key] === 'undefined') ? this[type][key] = value : false
  }

  debug.refresh = function(){ return registrants && transactions ? (this.update(), this) : this }

  debug.sum_balance = function( balance ) {
    return get_registrants_accepted().map( registrant => { return registrant.balance[balance] } ).filter( balance => { return web3.toBigNumber(balance).gt(0) } ).reduce( (sum, balance) => { return balance.plus(sum) }, web3.toBigNumber(0))
  } 

  debug.sum_balance_all = function() {
    return get_registrants_accepted().filter( registrant => registrant.accepted ).map( registrant => { return registrant.balance.total } ).filter( balance => { return web3.toBigNumber(balance).gt(0) } ).reduce( (sum, balance) => { return web3.toBigNumber(balance).plus(sum) }, web3.toBigNumber(0))    
  } 

  debug.sum_rejection = function( type ) {
    return get_registrants_rejected().filter( reject => { return reject.error == type } ).map( reject => { return reject.balance.total }).reduce( ( acc, balance)  => balance.plus(acc), web3.toBigNumber(0)  )
  }

  debug.sum_rejection_occurences = function( type ) {
    return get_registrants_rejected().filter( reject => { return reject.error == type } ).length
  }

  debug.output = function(){

    this.refresh();

    let _registry = new AsciiTable('Registry')

      _registry
        .setAlign(0, AsciiTable.RIGHT)
         .setAlign(1, AsciiTable.RIGHT)
        .addRow('% Complete', this.rates.percent_complete+'%')
        .addRow('Accepted',   this.registry.accepted )
        .addRow('Rejected',   this.registry.rejected )
        .addRow('Total',      this.registry.total )
        .setJustify()

    log("info", _registry.toString())

    let _distribution = new AsciiTable('Token Sums')
      _distribution
        .setAlign(0, AsciiTable.RIGHT)
         .setAlign(1, AsciiTable.RIGHT)
        .addRow('Total Supply', this.distribution.$total_supply.toFormat(4))
        .addRow('Snapshot Supply', this.distribution.$snapshot_supply.toFormat(4))
        .addRow(' -------------- ',' -------------- ')
        .addRow('Unclaimed', this.distribution.$balance_unclaimed.toFormat(4))
        .addRow('In Wallets', this.distribution.$balance_wallets.toFormat(4))
        .addRow('Reclaimed', this.distribution.$balance_reclaimed.toFormat(4))
        .addRow(' ------------- ',' -------------- ')
        .addRow('Total Rejected', this.rejected.$total.toFormat(4))
        .addRow('Total Included', this.distribution.$balances_found.toFormat(4))
        .addRow('Total Excluded', this.distribution.$balances_missing.toFormat(4))
        .addRow('Total Reclaimable', this.distribution.$balance_reclaimable.toFormat(4))

    log("info", _distribution.toString())

    let _rates = new AsciiTable('Overview')
      _rates
        .setAlign(0, AsciiTable.RIGHT)
         .setAlign(1, AsciiTable.RIGHT)
        .addRow('% Registrants Accepted',           this.rates.registrant_acceptance+'%')
        .addRow('% of Tokens Found',                this.rates.balances_found+'%')
        .addRow('% of Snapshot Supply Reclaimable', this.rates.balance_reclaimable+'%')
        .addRow('% of Found in Wallets',            this.rates.balance_wallets+'%')
        .addRow('% of Found is Unclaimed',          this.rates.balance_unclaimed+'%')
        .addRow('% of Found is Reclaimed',          this.rates.balance_reclaimed+'%')
        .addRow('% of Reclaimed Success',           this.rates.balance_reclaimed_success+'%')
      
   log("info", _rates.toString())

   let _sanity = new AsciiTable('Sanity')
      _sanity
        .setAlign(0, AsciiTable.RIGHT)
        .setAlign(1, AsciiTable.RIGHT)
        .addRow('Balances Totaled', `${ this.tests.balances }`)
        .addRow('Precision Loss', `${ this.tests.precision_loss }`)
        .addRow('Registrants Unique', `${ this.tests.unique }`)

    log("info", _sanity.toString())

    if(get_transactions_reclaimed().length) {
      let _reclaimed = new AsciiTable('Recovered')
      _reclaimed
          .setHeading(`ETH Address`, `EOS Key`, `Amount`, `TX`)
      for( let tx of get_transactions_reclaimed() ) {
        _reclaimed.addRow(tx.eth, tx.eos, tx.amount.div(WAD).toFormat(4), `http://etherscan.io/tx/${tx.hash}`)
      }
      log("info", _reclaimed.toString())
    }

  }

  return debug

}