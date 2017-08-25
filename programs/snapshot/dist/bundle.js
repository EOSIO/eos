const CROWDSALE_ABI = [{ "constant": true, "inputs": [{ "name": "", "type": "uint256" }, { "name": "", "type": "address" }], "name": "claimed", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "owner_", "type": "address" }], "name": "setOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "time", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "totalSupply", "outputs": [{ "name": "", "type": "uint128" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "foundersAllocation", "outputs": [{ "name": "", "type": "uint128" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "day", "type": "uint256" }], "name": "claim", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "foundersKey", "outputs": [{ "name": "", "type": "string" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "", "type": "uint256" }, { "name": "", "type": "address" }], "name": "userBuys", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "day", "type": "uint256" }], "name": "createOnDay", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [], "name": "freeze", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "", "type": "address" }], "name": "keys", "outputs": [{ "name": "", "type": "string" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "startTime", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "authority_", "type": "address" }], "name": "setAuthority", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "", "type": "uint256" }], "name": "dailyTotals", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "owner", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [], "name": "buy", "outputs": [], "payable": true, "type": "function" }, { "constant": true, "inputs": [], "name": "openTime", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "EOS", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "today", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "authority", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "eos", "type": "address" }], "name": "initialize", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "createFirstDay", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [], "name": "claimAll", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "timestamp", "type": "uint256" }], "name": "dayFor", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "day", "type": "uint256" }, { "name": "limit", "type": "uint256" }], "name": "buyWithLimit", "outputs": [], "payable": true, "type": "function" }, { "constant": false, "inputs": [], "name": "collect", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "numberOfDays", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "key", "type": "string" }], "name": "register", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "createPerDay", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "inputs": [{ "name": "_numberOfDays", "type": "uint256" }, { "name": "_totalSupply", "type": "uint128" }, { "name": "_openTime", "type": "uint256" }, { "name": "_startTime", "type": "uint256" }, { "name": "_foundersAllocation", "type": "uint128" }, { "name": "_foundersKey", "type": "string" }], "payable": false, "type": "constructor" }, { "payable": true, "type": "fallback" }, { "anonymous": false, "inputs": [{ "indexed": false, "name": "window", "type": "uint256" }, { "indexed": false, "name": "user", "type": "address" }, { "indexed": false, "name": "amount", "type": "uint256" }], "name": "LogBuy", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": false, "name": "window", "type": "uint256" }, { "indexed": false, "name": "user", "type": "address" }, { "indexed": false, "name": "amount", "type": "uint256" }], "name": "LogClaim", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": false, "name": "user", "type": "address" }, { "indexed": false, "name": "key", "type": "string" }], "name": "LogRegister", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": false, "name": "amount", "type": "uint256" }], "name": "LogCollect", "type": "event" }, { "anonymous": false, "inputs": [], "name": "LogFreeze", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "authority", "type": "address" }], "name": "LogSetAuthority", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "owner", "type": "address" }], "name": "LogSetOwner", "type": "event" }];
const TOKEN_ABI = [{ "constant": true, "inputs": [], "name": "name", "outputs": [{ "name": "", "type": "bytes32" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [], "name": "stop", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "guy", "type": "address" }, { "name": "wad", "type": "uint256" }], "name": "approve", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "owner_", "type": "address" }], "name": "setOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "totalSupply", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "src", "type": "address" }, { "name": "dst", "type": "address" }, { "name": "wad", "type": "uint256" }], "name": "transferFrom", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "decimals", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "dst", "type": "address" }, { "name": "wad", "type": "uint128" }], "name": "push", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "name_", "type": "bytes32" }], "name": "setName", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "wad", "type": "uint128" }], "name": "mint", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "src", "type": "address" }], "name": "balanceOf", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "stopped", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "authority_", "type": "address" }], "name": "setAuthority", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "src", "type": "address" }, { "name": "wad", "type": "uint128" }], "name": "pull", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "owner", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "wad", "type": "uint128" }], "name": "burn", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "symbol", "outputs": [{ "name": "", "type": "bytes32" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [{ "name": "dst", "type": "address" }, { "name": "wad", "type": "uint256" }], "name": "transfer", "outputs": [{ "name": "", "type": "bool" }], "payable": false, "type": "function" }, { "constant": false, "inputs": [], "name": "start", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "authority", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "src", "type": "address" }, { "name": "guy", "type": "address" }], "name": "allowance", "outputs": [{ "name": "", "type": "uint256" }], "payable": false, "type": "function" }, { "inputs": [{ "name": "symbol_", "type": "bytes32" }], "payable": false, "type": "constructor" }, { "anonymous": true, "inputs": [{ "indexed": true, "name": "sig", "type": "bytes4" }, { "indexed": true, "name": "guy", "type": "address" }, { "indexed": true, "name": "foo", "type": "bytes32" }, { "indexed": true, "name": "bar", "type": "bytes32" }, { "indexed": false, "name": "wad", "type": "uint256" }, { "indexed": false, "name": "fax", "type": "bytes" }], "name": "LogNote", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "authority", "type": "address" }], "name": "LogSetAuthority", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "owner", "type": "address" }], "name": "LogSetOwner", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "from", "type": "address" }, { "indexed": true, "name": "to", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" }], "name": "Transfer", "type": "event" }, { "anonymous": false, "inputs": [{ "indexed": true, "name": "owner", "type": "address" }, { "indexed": true, "name": "spender", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" }], "name": "Approval", "type": "event" }];
const UTILITY_ABI = [{ "constant": true, "inputs": [{ "name": "user", "type": "address" }], "name": "userBuys", "outputs": [{ "name": "result", "type": "uint256[351]" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "sale", "outputs": [{ "name": "", "type": "address" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "dailyTotals", "outputs": [{ "name": "result", "type": "uint256[351]" }], "payable": false, "type": "function" }, { "constant": true, "inputs": [{ "name": "user", "type": "address" }], "name": "userClaims", "outputs": [{ "name": "result", "type": "bool[351]" }], "payable": false, "type": "function" }, { "inputs": [{ "name": "_sale", "type": "address" }], "payable": false, "type": "constructor" }];
//For debugging and repurposed for gui
const calculator = function () {

  let debug = {};

  debug.tests = {};

  debug.registry = {
    total: 0 // Total Users found in LogRegister
    , accepted: 0 // Registrants with valid keys and balance
    , rejected: 0 // Registrants with invalid keys or zero balances
  };

  debug.reclaimable = {
    total: 0
  };

  debug.rejected = {
    balance_is_zero: 0 // Registrants with no balance
    , key_is_empty: 0 // key===""
    , key_is_eth: 0 // Ethereum key 0x...
    , key_is_malformed: 0 // Starts with EOS... but isn't a key
    , key_is_junk: 0 // Garbage
    , total: 0,
    $key_is_empty: web3.toBigNumber(0),
    $key_is_eth: web3.toBigNumber(0),
    $key_is_malformed: web3.toBigNumber(0),
    $key_is_junk: web3.toBigNumber(0),
    $total: web3.toBigNumber(0) // ^^^^^^ sum
  };

  debug.distribution = {
    $total_supply: web3.toBigNumber(0) // 2M(periods-1)+200M (not static for testnet snapshot)
    , $snapshot_supply: web3.toBigNumber(0) // 2M(periods-1)+200M (not static for testnet snapshot)
    , $balance_wallets: web3.toBigNumber(0) // sum(tokens in registrant walthis.balances)
    , $balance_unclaimed: web3.toBigNumber(0) // sum(tokens still locked in contract)
    , $balance_reclaimable: web3.toBigNumber(0) // crowdsale contract EOS token balance + token contract EOS token balance
    , $balance_reclaimed: web3.toBigNumber(0) // crowdsale contract EOS token balance + token contract EOS token balance
    , $balance_rejected: web3.toBigNumber(0) // sum(tokens in registrant walthis.balances)
    , $balances_found: web3.toBigNumber(0) // sum(wallet, unclaimed, reclaimable)
    , $balances_missing: web3.toBigNumber(0) // (tokens_supply-balances_found)/totens_total
  };

  debug.rates = {
    registrant_acceptance: null // registrants.accepted / registrants.total
    , balance_wallets: null // distribution.balance_wallets / distribution.balances_found
    , balance_unclaimed: null // 
    , balance_reclaimable: null //
    , balance_reclaimed: null //
    , balances_found: null // sum(wallet, unclaimed, reclaimable)
    , balances_missing: null // (tokens_supply-balances_found)/totens_total
  };

  debug.update = function () {
    //registrants
    this.registry.total = registrants.length;
    this.registry.accepted = get_registrants_accepted().length;
    this.registry.rejected = get_registrants_rejected().length;

    //Rejection Token Value
    this.rejected.$key_is_empty = this.sum_rejection("key_is_empty");
    this.rejected.$key_is_eth = this.sum_rejection("key_is_eth");
    this.rejected.$key_is_malformed = this.sum_rejection("key_is_malformed");
    this.rejected.$key_is_junk = this.sum_rejection("key_is_junk");
    this.rejected.$total = get_registrants_rejected().filter(reject => {
      return reject.rejected != "balance_is_zero";
    }).map(reject => {
      return reject.balance.total;
    }).reduce((acc, balance) => balance.plus(acc), web3.toBigNumber(0));

    //Rejection Occurences
    this.rejected.balance_is_zero = this.sum_rejection_occurences("balance_is_zero");
    this.rejected.key_is_empty = this.sum_rejection_occurences("key_is_empty");
    this.rejected.key_is_eth = this.sum_rejection_occurences("key_is_eth");
    this.rejected.key_is_malformed = this.sum_rejection_occurences("key_is_malformed");
    this.rejected.key_is_junk = this.sum_rejection_occurences("key_is_junk");

    //Distribution 1

    this.distribution.$total_supply = web3.toBigNumber(get_supply()).div(WAD);
    this.distribution.$snapshot_supply = web3.toBigNumber(get_supply(period_for(SS_LAST_BLOCK_TIME))).div(WAD);

    this.distribution.$balances_found = this.sum_balance_all();
    this.distribution.$balance_wallets = this.sum_balance('wallet');
    this.distribution.$balance_unclaimed = this.sum_balance('unclaimed');
    this.distribution.$balance_reclaimed = this.sum_balance('reclaimed');

    //Distribution
    this.distribution.$balances_missing = this.distribution.$snapshot_supply.minus(this.distribution.$balances_found);
    this.distribution.$balance_reclaimable = get_transactions_reclaimable().reduce((sum, tx) => {
      return tx.amount.div(WAD).plus(sum);
    }, web3.toBigNumber(0));
    this.reclaimable.total = get_transactions_reclaimable().length;

    //Rates
    this.rates.percent_complete = to_percent((this.registry.accepted + this.registry.rejected) / this.registry.total);
    this.rates.balances_found = to_percent(this.distribution.$balances_found.div(this.distribution.$snapshot_supply).toFixed(2));
    this.rates.registrant_acceptance = to_percent(this.registry.accepted / (this.registry.accepted + this.registry.rejected));
    this.rates.balance_wallets = to_percent(this.distribution.$balance_wallets.div(this.distribution.$balances_found).toFixed(2));
    this.rates.balance_unclaimed = to_percent(this.distribution.$balance_unclaimed.div(this.distribution.$balances_found).toFixed(2));
    this.rates.balance_reclaimed = to_percent(this.distribution.$balance_reclaimed.div(this.distribution.$balances_found).toFixed(2));
    this.rates.balance_reclaimable = to_percent(this.distribution.$balance_reclaimable.div(this.distribution.$snapshot_supply).toFixed(2));
    this.rates.balance_reclaimed_success = to_percent(this.distribution.$balance_reclaimed.div(this.distribution.$balance_reclaimable.plus(this.distribution.$balance_reclaimed)).toFixed(2));
    this.rates.balance_reclaimable_total = to_percent(this.distribution.$balance_reclaimable.div(this.distribution.$total_supply).toFixed(2));

    //Tests
    this.tests.balances = this.distribution.$balance_wallets.plus(this.distribution.$balance_unclaimed).plus(this.distribution.$balance_reclaimed).eq(this.distribution.$balances_found) ? "PASS" : `FAIL ${sum.toFixed(4)} != ${this.distribution.$balances_found.toFixed(4)}`;
    this.tests.precision_loss = this.distribution.$balance_wallets.plus(this.distribution.$balance_unclaimed).plus(this.distribution.$balance_reclaimed).minus(this.distribution.$balances_found);

    let _registrants = new Set();
    this.tests.unique = !registrants.some(registrant => _registrants.size === _registrants.add(registrant.eth).size) ? "PASS" : "FAIL";
  };

  debug.find = function (group, key) {
    return typeof this[group] === 'object' && typeof this[type][key] !== 'undefined' ? this[type][key] : web3.toBigNumber(0);
  };

  debug.set = function (group, key, value) {
    typeof this[group] === 'object' && typeof this[type][key] === 'undefined' ? this[type][key] = value : false;
  };

  debug.refresh = function () {
    return registrants && transactions ? (this.update(), this) : this;
  };

  debug.sum_balance = function (balance) {
    return get_registrants_accepted().map(registrant => {
      return registrant.balance[balance];
    }).filter(balance => {
      return web3.toBigNumber(balance).gt(0);
    }).reduce((sum, balance) => {
      return balance.plus(sum);
    }, web3.toBigNumber(0));
  };

  debug.sum_balance_all = function () {
    return get_registrants_accepted().filter(registrant => registrant.accepted).map(registrant => {
      return registrant.balance.total;
    }).filter(balance => {
      return web3.toBigNumber(balance).gt(0);
    }).reduce((sum, balance) => {
      return web3.toBigNumber(balance).plus(sum);
    }, web3.toBigNumber(0));
  };

  debug.sum_rejection = function (type) {
    return get_registrants_rejected().filter(reject => {
      return reject.error == type;
    }).map(reject => {
      return reject.balance.total;
    }).reduce((acc, balance) => balance.plus(acc), web3.toBigNumber(0));
  };

  debug.sum_rejection_occurences = function (type) {
    return get_registrants_rejected().filter(reject => {
      return reject.error == type;
    }).length;
  };

  debug.output = function () {

    this.refresh();

    let _registry = new AsciiTable('Registry');

    _registry.setAlign(0, AsciiTable.RIGHT).setAlign(1, AsciiTable.RIGHT).addRow('% Complete', this.rates.percent_complete + '%').addRow('Accepted', this.registry.accepted).addRow('Rejected', this.registry.rejected).addRow('Total', this.registry.total).setJustify();

    log("info", _registry.toString());

    let _distribution = new AsciiTable('Token Sums');
    _distribution.setAlign(0, AsciiTable.RIGHT).setAlign(1, AsciiTable.RIGHT).addRow('Total Supply', this.distribution.$total_supply.toFormat(4)).addRow('Snapshot Supply', this.distribution.$snapshot_supply.toFormat(4)).addRow(' -------------- ', ' -------------- ').addRow('Unclaimed', this.distribution.$balance_unclaimed.toFormat(4)).addRow('In Wallets', this.distribution.$balance_wallets.toFormat(4)).addRow('Reclaimed', this.distribution.$balance_reclaimed.toFormat(4)).addRow(' ------------- ', ' -------------- ').addRow('Total Rejected', this.rejected.$total.toFormat(4)).addRow('Total Included', this.distribution.$balances_found.toFormat(4)).addRow('Total Excluded', this.distribution.$balances_missing.toFormat(4)).addRow('Total Reclaimable', this.distribution.$balance_reclaimable.toFormat(4));

    log("info", _distribution.toString());

    let _rates = new AsciiTable('Overview');
    _rates.setAlign(0, AsciiTable.RIGHT).setAlign(1, AsciiTable.RIGHT).addRow('% Registrants Accepted', this.rates.registrant_acceptance + '%').addRow('% of Tokens Found', this.rates.balances_found + '%').addRow('% of Snapshot Supply Reclaimable', this.rates.balance_reclaimable + '%').addRow('% of Found in Wallets', this.rates.balance_wallets + '%').addRow('% of Found is Unclaimed', this.rates.balance_unclaimed + '%').addRow('% of Found is Reclaimed', this.rates.balance_reclaimed + '%').addRow('% of Reclaimed Success', this.rates.balance_reclaimed_success + '%');

    log("info", _rates.toString());

    let _sanity = new AsciiTable('Sanity');
    _sanity.setAlign(0, AsciiTable.RIGHT).setAlign(1, AsciiTable.RIGHT).addRow('Balances Totaled', `${this.tests.balances}`).addRow('Precision Loss', `${this.tests.precision_loss}`).addRow('Registrants Unique', `${this.tests.unique}`);

    log("info", _sanity.toString());

    if (get_transactions_reclaimed().length) {
      let _reclaimed = new AsciiTable('Recovered');
      _reclaimed.setHeading(`ETH Address`, `EOS Key`, `Amount`, `TX`);
      for (let tx of get_transactions_reclaimed()) {
        _reclaimed.addRow(tx.eth, tx.eos, tx.amount.div(WAD).toFormat(4), `http://etherscan.io/tx/${tx.hash}`);
      }
      log("info", _reclaimed.toString());
    }
  };

  return debug;
};
class Registrant {

  constructor(eth, eos = "", balance = 0) {
    this.eth = eth;
    this.eos = eos;
    this.balance = typeof balance == 'object' ? balance : new Balance();

    this.index = null;
    this.accepted = null;
    this.error = false;
  }

  accept(callback) {

    this.accepted = true;
    log("message", `[#${this.index}] accepted ${this.eth} => ${this.eos} => ${this.balance.total.toFormat(4)}`);
  }

  reject() {
    this.accepted = false;
    let msg = "";

    if (this.balance.exists('reclaimed')) log("reject", `[#${this.index}] rejected ${this.eth} => ${this.eos} => ${this.balance.total.toFormat(4)} => ${this.error} ( ${this.balance.reclaimed.toFormat(4)} reclaimed EOS tokens moved back to Reclaimable )`);else log("reject", `[#${this.index}] rejected ${this.eth} => ${this.eos} => ${this.balance.total.toFormat(4)} => ${this.error}`);
  }

  judgement() {
    return this.valid() ? this.accept() : this.reject();
  }

  set(key, value) {
    return typeof this[`set_${key}`] === "function" ? this[`set_${key}`](value) : this;
  }

  set_index(index) {
    this.index = index;
    return this; //for chaining
  }

  set_key(eos_key) {
    //Might be hex, try to convert it.
    if (eos_key.length == 106) {
      let eos_key_from_hex = web3.toAscii(eos_key);
      if (eos_key_from_hex.startsWith('EOS') && eos_key_from_hex.length == 53) {
        eos_key = eos_key_from_hex;
      }
    }
    //Might be user error
    else if (eos_key.startsWith('key')) {
        let eos_key_mod = eos_key.substring(3);
        if (eos_key_mod.startsWith('EOS') && eos_key_mod.length == 53) {
          eos_key = eos_key_mod;
        }
      }
    this.eos = eos_key;
    return this; //for chaining
  }

  // Reject bad keys and zero balances, elseif was fastest? :/
  valid() {

    //Reject 0 balances
    if (this.balance.total.lt(1)) {
      this.error = 'balance_insufficient';
    }

    //Everything else
    else if (!this.eos.startsWith('EOS')) {

        //It's an empty key
        if (this.eos.length == 0) {
          this.error = 'key_is_empty';
        }

        //It may be an EOS private key
        else if (this.eos.startsWith('5')) {
            this.error = 'key_is_private';
          }

          // It almost looks like an EOS key // #TODO ACTUALLY VALIDATE KEY?
          else if (this.eos.startsWith('EOS') && this.eos.length != 53) {
              this.error = 'key_is_malformed';
            }

            // ETH address
            else if (this.eos.startsWith('0x')) {
                this.error = 'key_is_eth';
              }

              //Reject everything else with junk label
              else {
                  this.error = 'key_is_junk';
                }
      }

    //Accept BTS and STM keys, assume advanced users and correct format
    if (this.eos.startsWith('BTS') || this.eos.startsWith('STM')) {
      this.error = false;
    }

    return !this.error ? true : false;
  }
}

class Balance {

  constructor() {
    this.wallet = web3.toBigNumber(0);
    this.unclaimed = web3.toBigNumber(0);
    this.reclaimed = web3.toBigNumber(0);
    this.total = web3.toBigNumber(0);
  }

  set(type, balance) {
    this[type] = balance;
    return this; //chaining
  }

  readable(type = 'total') {
    this[type] = formatEOS(this[type]);
  }

  exists(type) {
    return typeof this[type] !== "undefined" && this[type].gt(0);
  }

  get(type) {
    return typeof this[type] !== "undefined" && this[type].gt(0) ? this[type] : false;
  }

  sum() {
    this.total = this.wallet.plus(this.unclaimed).plus(this.reclaimed);
    //Save some dust, higher accuracy. 
    this.total = this.total.div(WAD);
    this.wallet = this.wallet.div(WAD);
    this.unclaimed = this.unclaimed.div(WAD);
    this.reclaimed = this.reclaimed.div(WAD);
  }
}

class Transaction {

  constructor(eth, tx, type = "transfer", amount) {
    this.eth = eth;
    this.eos = null;
    this.hash = tx;
    this.amount = amount;
    this.claimed = false;
    this.type = type;
  }

  claim(eth) {
    return eth == this.eth ? (this.claimed = true, log("success", `reclaimed ${this.eth} => ${this.eos} => ${this.amount.div(WAD).toFormat(4)} EOS <<< tx: https://etherscan.io/tx/${this.hash}`)) : log("error", `${eth} should't be claiming ${this.eth}'s transaction`);
  }

}
//Change if needed
const NODE = "http://127.0.0.1:8545"; //Default Host

//testnet || mainnet
const SNAPSHOT_ENV = "testnet";

//User Options
let CONSOLE_LOGGING = true; // Push activity to developer console, WARNING: unstable in Firefox and Safari
let OUTPUT_LOGGING = false; // Generate an output log file (±6MB), WARNING: degrades browser performance significantly.
let VERBOSE_REGISTRY_LOGS = false; // Generate a log entry based on LogRegistry events, WARNING: degrades performance
let UI_SHOW_STATUS_EVERY = 150; // Shows some status information in console every X logs, useful for debugging.

// Needs to be set so all snapshots generated are the somewhat the same 
// Caveat: testnet snapshots will rarely be the same because wallet balances can still change, but setting last block *will* provide consistent registry totals.
const SS_FIRST_BLOCK = 3904416; //Block EOS Contract was deployed at
const SS_LAST_BLOCK = 4167293; //Last block to honor ethereum transactions [NOT FINAL]

//Test Case: Quick, Has orphaned TX, Has a Reclaimed Registrant and has two types of rejects.
// const SS_FIRST_BLOCK            = 4146970    //Block EOS Contract was deployed at
// const SS_LAST_BLOCK             = 4148293    //Last block to honor ethereum transactions

//For Web3 Init
const CROWDSALE_ADDRESS = "0xd0a6E6C54DbC68Db5db3A091B171A77407Ff7ccf"; //for Mapping
const TOKEN_ADDRESS = "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0"; //for balanceOf()
const UTILITY_ADDRESS = "0x860fd485f533b0348e413e65151f7ee993f93c02"; //Useful functions

//Crowdsale Figures
const WAD = 1000000000000000000;
const SS_BLOCK_ONE_DIST = 100000000 * WAD;
let SS_PERIOD_ETH;
let SS_LAST_BLOCK_TIME;

const CS_NUMBER_OF_PERIODS = 350;
const CS_CREATE_FIRST_PERIOD = 200000000 * WAD;
const CS_CREATE_PER_PERIOD = 2000000 * WAD;
const CS_START_TIME = 1498914000;
// CSVs
const download_reclaimable_csv = () => {
  let headers = {
    eth: "eth_address",
    tx: "tx",
    balance: "amount"
  };
  export_csv(headers, output.reclaimable, generate_filename('reclaimable-tx'));
};

const download_reclaimed_csv = () => {
  let headers = {
    eth: "eth_address",
    eos: "eos_key",
    tx: "tx",
    amount: "amount"
  };
  export_csv(headers, output.reclaimed, generate_filename('successfully-reclaimed-tx'));
};

const download_snapshot_csv = () => {
  let headers = {
    eth: "eth_address",
    eos: "eos_key",
    balance: "balance"
  };
  export_csv(headers, output.snapshot, generate_filename('snapshot'));
};

const download_rejects_csv = () => {
  let headers = {
    rejected: "error",
    eth: "eth_address",
    eos: "eos_key",
    balance: "balance"
  };
  export_csv(headers, output.rejects, generate_filename('rejections'));
};

// JSON
const download_reclaimable_json = () => export_json(output.reclaimable, generate_filename('reclaimable-tx'));
const download_reclaimed_json = () => export_json(output.reclaimed, generate_filename('successfully-reclaimed-tx'));
const download_snapshot_json = () => export_json(output.snapshot, generate_filename('snapshot'));
const download_rejects_json = () => export_json(output.rejects, generate_filename('rejections'));

// Logs
const download_logs = () => export_log(output.logs, generate_filename('logs'));

// Helper
const generate_filename = type => `eos-${type}_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`;

//https://codepen.io/danny_pule/pen/WRgqNx
const convert_to_csv = objArray => {
  let array = typeof objArray != 'object' ? JSON.parse(objArray) : objArray;
  let str = '';
  for (let i = 0; i < array.length; i++) {
    let line = '';
    for (let index in array[i]) {
      if (line != '') line += ',';
      line += array[i][index];
    }
    str += line + '\r\n';
  }
  return str;
};

const export_csv = (headers, items, filename) => {
  filename = filename + '.csv' || 'export.csv';
  if (headers) {
    items.unshift(headers);
  }
  let jsonObject = JSON.stringify(items);
  let csv = convert_to_csv(jsonObject);
  let blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
  if (navigator.msSaveBlob) {
    // IE 10+
    navigator.msSaveBlob(blob, filename);
  } else {
    let link = document.createElement("a");
    if (link.download !== undefined) {
      // feature detection
      // Browsers that support HTML5 download attribute
      let url = URL.createObjectURL(blob);
      link.setAttribute("href", url);
      link.setAttribute("download", filename);
      link.style.visibility = 'hidden';
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }
  }
};

const export_json = (items, filename) => {
  filename = filename + '.json' || 'export.csv';
  let json = JSON.stringify(items);
  let blob = new Blob([json], { type: 'application/json;charset=utf-8;' });
  if (navigator.msSaveBlob) {
    // IE 10+
    navigator.msSaveBlob(blob, filename);
  } else {
    let link = document.createElement("a");
    if (link.download !== undefined) {
      // feature detection
      // Browsers that support HTML5 download attribute
      let url = URL.createObjectURL(blob);
      link.setAttribute("href", url);
      link.setAttribute("download", filename);
      link.style.visibility = 'hidden';
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }
  }
};

const export_log = (logs, filename) => {
  filename = filename + '.log' || 'export.log';
  let log = logs.reduce((acc, log) => {
    return `${acc}${log}\r\n`;
  });
  let blob = new Blob([log], { type: 'text/plain;charset=utf-8;' });
  if (navigator.msSaveBlob) {
    // IE 10+
    navigator.msSaveBlob(blob, filename);
  } else {
    let link = document.createElement("a");
    if (link.download !== undefined) {
      // feature detection
      // Browsers that support HTML5 download attribute
      let url = URL.createObjectURL(blob);
      link.setAttribute("href", url);
      link.setAttribute("download", filename);
      link.style.visibility = 'hidden';
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }
  }
};
// For processing
let registrants = [],
    transactions = [];

// For Export
let output = {};
output.snapshot = [];
output.rejects = [];
output.reclaimable = [];
output.reclaimed = [];
output.logs = [];

// Web3
const Web3 = require('web3');
let web3;
let contract = {};

// Debugging
let debug;

window.onload = () => {
  init();
};

const init = () => {
  console.clear();
  log("large_text", 'EOS Token Distribution (testnet)');
  if (typeof console.time === 'function') console.time('Generated Snapshot');

  web3 = new Web3(new Web3.providers.HttpProvider(NODE));

  if (!web3.isConnected()) log("error", 'web3 is disconnected'), log("info", 'Please make sure you have a local ethereum node runnning on localhost:8345'), disconnected(init);else if (!is_synced()) log("error", `web3 is still syncing, retrying in 10 seconds. Current block: ${web3.eth.syncing.currentBlock}, Required Height: ${SS_LAST_BLOCK}`), node_syncing(init);else debug = new calculator(), contract.$crowdsale = web3.eth.contract(CROWDSALE_ABI).at(CROWDSALE_ADDRESS), contract.$token = web3.eth.contract(TOKEN_ABI).at(TOKEN_ADDRESS), contract.$utilities = web3.eth.contract(UTILITY_ABI).at(UTILITY_ADDRESS), SS_PERIOD_ETH = period_eth_balance(), SS_LAST_BLOCK_TIME = get_last_block_time(), async.series([scan_registry, find_reclaimables, distribute_tokens, verify, exporter], (error, results) => {
    if (!error) {
      log("success", 'Distribution List Complete');
      log('block', 'CSV Downloads Ready');
      if (typeof console.timeEnd === 'function') console.timeEnd('Generated Snapshot');
      setTimeout(() => {
        // Makes downloads less glitchy
        clearInterval(ui_refresh);
        registrants = undefined, transactions = undefined; //object literals have been generated, unset array of resource consuming referenced objects.
      }, 5000);
    } else {
      log("error", error);
    }
    debug.refresh().output();
  });
};

//Sets balances, validates registrant, adds to distribution if good  
const distribute_tokens = finish => {
  log('group', 'Distribution List');
  let index = 0;
  const total = registrants.length;
  const iterate = () => {
    if (!web3.isConnected()) return disconnected(reconnect = iterate);
    try {
      let registrant = registrants[index];

      registrant.set("index", index).set("key", contract.$crowdsale.keys(registrant.eth))
      // Every registrant has three different balances, 
      // Wallet:      Tokens in Wallet
      // Unclaimed:   Tokens in contract
      // Reclaimed:   Tokens sent to crowdsale/token contracts
      .balance
      // Ask token contract what this user's EOS balance is
      .set('wallet', web3.toBigNumber(contract.$token.balanceOf(registrant.eth)))
      // Loop through periods and calculate unclaimed
      .set('unclaimed', web3.toBigNumber(sum_unclaimed(registrant)))
      // Check reclaimable index for ethereum user, loop through tx
      .set('reclaimed', web3.toBigNumber(maybe_reclaim_tokens(registrant)))
      // wallet+unclaimed+reclaimed
      .sum();
      // Reject or Accept
      registrant.judgement();
      status_log();
      // Runaway loops, calls iterate again or calls finish() callback from async
      setTimeout(() => {
        index++, index < total ? iterate() : (console.groupEnd(), finish(null, true));
      }, 1);
    }
    // This error is web3 related, try again and resume if successful
    catch (e) {
      log("error", e);
      if (!web3.isConnected()) {
        log("message", `Attempting reconnect once per second, will resume from registrant ${index}`);
        disconnected(reconnect = iterate);
      }
    } finally {}
  };

  const sum_unclaimed = registrant => {
    //Find all Buys
    let buys = contract.$utilities.userBuys(registrant.eth).map(web3.toBigNumber);
    //Find Claimed Balances
    let claims = contract.$utilities.userClaims(registrant.eth);
    //Compile the periods, and user parameters for each period for later reduction
    const periods = iota(Number(CS_NUMBER_OF_PERIODS) + 1).map(i => {
      let period = {};
      period.tokens_available = web3.toBigNumber(period == 0 ? CS_CREATE_FIRST_PERIOD : CS_CREATE_PER_PERIOD);
      period.total_eth = SS_PERIOD_ETH[i];
      period.buys = buys[i] && buys[i];
      period.share = !period.buys || period.total_eth.equals(0) ? web3.toBigNumber(0) : period.tokens_available.div(period.total_eth).times(period.buys);
      period.claimed = claims && claims[i];
      return period;
    });

    return web3.toBigNumber(periods
    //Get periods by unclaimed and lte last block
    .filter((period, i) => {
      return i <= period_for(SS_LAST_BLOCK_TIME) && !period.claimed;
    })
    //Sum the pre-calculated EOS balance of each resulting period
    .reduce((sum, period) => period.share.plus(sum), web3.toBigNumber(0)));
  };

  // Some users mistakenly sent their tokens to the contract or token address. Here we recover them if it is the case. 
  const maybe_reclaim_tokens = registrant => {
    let reclaimed_balance = web3.toBigNumber(0);
    let reclaimable_transactions = get_registrant_reclaimable(registrant.eth);
    if (reclaimable_transactions.length) {
      for (let tx of reclaimable_transactions) {
        reclaimed_balance = tx.amount.plus(reclaimed_balance);
        tx.eos = registrant.eos;
        tx.claim(registrant.eth);
      }
      if (reclaimable_transactions.length > 1) log("info", `Reclaimed ${reclaimed_balance.div(WAD).toFormat(4)} EOS from ${reclaimable_transactions.length} tx for ${registrant.eth} => ${registrant.eos}`);
    }
    return reclaimed_balance;
  };

  const status_log = () => {
    if (index % UI_SHOW_STATUS_EVERY == 0 && index != 0) debug.refresh(), console.group("Snapshot Status"), log("success", `Progress:      ${debug.rates.percent_complete}% `), log("success", `---------------------------------`), log("success", `Unclaimed:     EOS ${debug.distribution.$balance_unclaimed.toFormat(4)}`), log("success", `In Wallets:    EOS ${debug.distribution.$balance_wallets.toFormat(4)}`), log("success", `Reclaimed:     EOS ${debug.distribution.$balance_reclaimed.toFormat(4)}`), log("success", `Total:         EOS ${debug.distribution.$balances_found.toFormat(4)}`), log("error", `Rejected:      EOS ${debug.rejected.$total.toFormat(4)}`), log("error", `Reclaimable:   EOS ${debug.distribution.$balance_reclaimable.toFormat(4)}`), log("error", `Not Included:  EOS ${debug.distribution.$balances_missing.toFormat(4)}`), console.groupEnd();
  };

  iterate();
};

// Crowdsale contract didnt store registrants in an array, so we need to scrap LogRegister using a polling function :/ 

// Builds Registrants List
const scan_registry = on_complete => {

  console.group('EOS Registrant List');

  let registry_logs = [];
  let message_freq = 10000;
  let last_message = Date.now();

  let group_index = 1;

  const on_result = log_register => {
    //Since this is a watcher, we need to be sure the result isn't new. 
    if (log_register.blockNumber <= SS_LAST_BLOCK) {

      //check that registrant isn't already in array
      if (!registrants.filter(registrant => {
        return registrant.eth == log_register.args.user;
      }).length) {

        let registrant = new Registrant(log_register.args.user);
        registrants.push(registrant);

        //Add the register transaction to transactions array, may be unnecessary. 
        // let tx = new Transaction( registrant.eth, log_register.transactionHash, 'register', web3.toBigNumber(0) )
        // transactions.push(tx)

        registry_logs.push(`${registrant.eth} => ${log_register.args.key} (pending) => https://etherscan.io/tx/${log_register.transactionHash}`);

        //now all your snowflakes are urine...
        maybe_log();
      }
    }
  };

  const on_success = () => {
    debug.refresh(), log_group(), console.groupEnd(), log("block", `${registrants.length} Total Registrants`), { found: Object.keys(registrants).length };
  };

  const on_error = () => "Web3's watcher misbehaved while chugging along in the registry";

  let maybe_log = () => Date.now() - message_freq > last_message ? (log_group(), true) : false;

  //...and you can’t even find the cat.
  let log_group = () => {
    let group_msg = `${group_index}. ... ${registry_logs.length} Found, Total: ${registrants.length}`;
    log("groupCollapsed", group_msg), OUTPUT_LOGGING ? output.logs.push(group_msg) : null, registry_logs.forEach(_log => {
      if (VERBOSE_REGISTRY_LOGS) log("message", _log);
      if (OUTPUT_LOGGING) output.logs.push(_log);
    }), registry_logs = [], console.groupEnd(), group_index++, last_message = Date.now();
  };

  let async_watcher = LogWatcher(SS_FIRST_BLOCK, SS_LAST_BLOCK, contract.$crowdsale.LogRegister, null, on_complete, on_result, on_success, on_error, 2500, 5000);
};

//Builds list of EOS tokens sent to EOS crowdsale or token contract.

const find_reclaimables = on_complete => {
  log('group', 'Finding Reclaimable EOS Tokens');
  let index = 0;
  const query = { to: ['0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0', '0xd0a6E6C54DbC68Db5db3A091B171A77407Ff7ccf'] };

  const on_result = reclaimable_tx => {
    if (reclaimable_tx.blockNumber <= SS_LAST_BLOCK && reclaimable_tx.args.value.gt(web3.toBigNumber(0))) {
      let tx = new Transaction(reclaimable_tx.args.from, reclaimable_tx.transactionHash, 'transfer', reclaimable_tx.args.value);
      transactions.push(tx);
      log("error", `${tx.eth} => ${web3.toBigNumber(tx.amount).div(WAD)} https://etherscan.io/tx/${tx.hash}`);
    }
  };

  const on_success = () => {
    let result_total = get_transactions_reclaimable().length;
    debug.refresh(), console.groupEnd(), log("block", `${result_total || 0} reclaimable transactions found`), { found: result_total };
  };

  const on_error = () => "Something bad happened while finding reclaimables";

  let async_watcher = LogWatcher(SS_FIRST_BLOCK, SS_LAST_BLOCK, contract.$token.Transfer, query, on_complete, on_result, on_success, on_error, 500, 5000);
};

const verify = callback => {
  let valid = true;
  if (![...new Set(get_registrants_accepted().map(registrant => registrant.eth))].length == get_registrants_accepted().length) log("error", 'Duplicate Entry Found'), valid = false;
  return valid ? callback(null, valid) && true : callback(true) && false;
};

const exporter = callback => {
  if (typeof console.time === 'function') console.time('Compiled Data to Output Arrays');

  output.reclaimable = get_transactions_reclaimable().map(tx => {
    return { eth: tx.eth, tx: tx.hash, amount: formatEOS(web3.toBigNumber(tx.amount).div(WAD)) };
  });
  output.reclaimed = get_transactions_reclaimed().map(tx => {
    return { eth: tx.eth, eos: tx.eos, tx: tx.hash, amount: formatEOS(web3.toBigNumber(tx.amount).div(WAD)) };
  });
  output.snapshot = get_registrants_accepted().map(registrant => {
    return { eth: registrant.eth, eos: registrant.eos, balance: formatEOS(registrant.balance.total) };
  });
  output.rejects = get_registrants_rejected().map(registrant => {
    return { error: registrant.error, eth: registrant.eth, eos: registrant.eos, balance: formatEOS(registrant.balance.total) };
  });

  if (typeof console.timeEnd === 'function') console.timeEnd('Compiled Data to Output Arrays');

  callback(null, true);
};
const unhex = data => data.replace(/^0x/, "");
const hex = data => `0x${unhex(data)}`;
const bytes4 = data => hex(unhex(data).slice(0, 8));
const sighash = sig => bytes4(web3.sha3(sig));
const parts = (data, n) => data.match(new RegExp(`.{${n}}`, "g")) || [];
const hexparts = (data, n) => parts(unhex(data), n).map(hex);
const words32 = data => hexparts(data, 64);
const bytes = data => hexparts(data, 2);
const hexcat = (...data) => hex(data.map(unhex).join(""));
const word = data => hex(unhex(data).padStart(64, "0"));
const calldata = (sig, ...words) => hexcat(sig, ...words.map(word));
const toAsync = promise => $ => promise.then(x => $(null, x), $);
const byId = id => document.getElementById(id);
const formatWad = wad => String(wad).replace(/\..*/, "");
const formatEOS = wad => wad ? wad.toFormat(4).replace(/,/g, "") : "0";
const formatETH = wad => wad ? wad.toFormat(2).replace(/,/g, "") : "0";
const getValue = id => byId(id).value;
const show = id => byId(id).classList.remove("hidden");
const hide = id => byId(id).classList.add("hidden");
const iota = n => repeat("x", n).split("").map((_, i) => i);
const repeat = (x, n) => new Array(n + 1).join("x");
const getTime = () => new Date().getTime() / 1000;

// Getters

// Transactions
const get_transactions_reclaimable = () => transactions.filter(tx => tx.type == 'transfer' && tx.amount.gt(0) && (!tx.claimed || tx.claimed && get_registrants_rejected().filter(registrant => registrant.eth == tx.eth).length));
const get_transactions_reclaimed = () => transactions.filter(tx => tx.claimed && get_registrants_accepted().filter(registrant => registrant.eth == tx.eth).length);

// Registrant Collection
const get_registrants_accepted = () => registrants.filter(registrant => registrant.accepted);
const get_registrants_rejected = () => registrants.filter(registrant => registrant.accepted === false);
const get_registrants_unprocessed = () => registrants.filter(registrant => registrant.accepted === null);

// Registrant
const get_registrant_reclaimed = eth => transactions.filter(tx => tx.type == 'transfer' && tx.claimed && eth == tx.eth);
const get_registrant_reclaimable = eth => transactions.filter(tx => tx.type == 'transfer' && !tx.claimed && eth == tx.eth && tx.amount.gt(0));

// Web3
const node_syncing = (callback = () => {}) => setTimeout(() => {
  is_synced() ? callback() : node_syncing(callback);
}, 3000);
const disconnected = (callback = () => {}) => setTimeout(() => {
  is_listening() ? callback() : disconnected(callback);
}, 3000);
const is_synced = () => !web3.eth.syncing ? true : web3.eth.syncing.currentBlock > SS_LAST_BLOCK ? true : false;

// Crowdsale Helpers
const period_for = timestamp => timestamp < CS_START_TIME ? 0 : Math.min(Math.floor((timestamp - CS_START_TIME) / 23 / 60 / 60) + 1, 350);
const period_eth_balance = () => contract.$utilities.dailyTotals().map(web3.toBigNumber);
const get_last_block_time = () => web3.eth.getBlock(SS_LAST_BLOCK).timestamp;
const get_supply = (periods = 350) => web3.toBigNumber(CS_CREATE_PER_PERIOD).times(periods).plus(web3.toBigNumber(CS_CREATE_FIRST_PERIOD)).plus(web3.toBigNumber(SS_BLOCK_ONE_DIST));
const to_percent = result => Math.floor(result * 100);

// Check if everything is working
const is_listening = () => {
  let listening = true;
  try {
    web3.isConnected();
  } catch (e) {
    listening = false;
  }
  return listening;
};

//logger helper
const log = (type, msg = "") => {
  if (msg.length && OUTPUT_LOGGING) output.logs.push(msg);
  if (console && console.log && CONSOLE_LOGGING && typeof logger[type] !== 'undefined') logger[type](msg);
};

const logger = {};
logger.success = msg => console.log(`%c ${msg}`, 'font-weight:bold; color:green; background:#DEF6E6; ');
logger.error = msg => console.log(`%c ${msg}`, 'font-weight:bold; color:red;');
logger.bold = msg => console.log(`%c ${msg}`, 'font-weight:bold;');
logger.info = msg => console.log(`%c ${msg}`, 'font-style:italic; color:#888;');
logger.block_error = msg => console.log(`%c ${msg}`, 'width:100%; background-color:red; color:#FFF; font-weight:bold; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:9pt');
logger.block_success = msg => console.log(`%c ${msg}`, 'width:100%; background-color:green; color:#FFF; font-weight:bold; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:9pt');
logger.debug = msg => console.log(`%c ${msg}`, log_style_debug);
logger.message = msg => console.log(`%c ${msg}`, 'color:#666');
logger.reject = msg => console.log(`%c ${msg}`, 'color:#b0b0b0; text-decoration:line-through; font-style:italic; ');
logger.block = msg => console.log(`%c ${msg}`, 'width:100%; background-color:black; color:#FFF; font-weight:bold; line-height:30px; letter-spacing:0.5px; display:inline-block; padding:3px 10px; font-size:10pt');
logger.largest_text = msg => console.log(`%c ${msg}`, 'line-height:45px; font-size:33pt; color:#dedede; font-weight:100; display:inline-block ');
logger.large_text = msg => console.log(`%c ${msg}`, 'line-height:45px; font-size:14pt; color:#c0c0c0; font-weight:100; display:inline-block ');
logger.group = msg => console.group(msg);
logger.groupCollapsed = msg => console.groupCollapsed(msg);
logger.groupEnd = () => console.groupEnd();
//minimal UI for now calls for vanilla js and some helpers for cleaner business logic
let ui_refresh;

(function () {

  let current_step;

  // Enable navigation prompt
  window.onbeforeunload = function () {
    return true;
  };

  // document.title updates
  const update_title_bar_progress = (stage = "") => {
    let title;
    switch (stage) {

      case 'distribute':
        title = `EOS | Snapshot ${debug.rates.percent_complete}% | ${debug.registry.accepted + debug.registry.rejected}/${debug.registry.total}`;
        break;
      case 'registry':
        title = `EOS | Building Registry | ${registrants.length}`;
        break;
      case 'reclaimer':
        title = `EOS | Reclaimable TXs | ${debug.reclaimable.total}`;
        break;
      case 'complete':
        title = `EOS | Snapshot Complete`;
        break;
      default:
        title = `EOS | Welcome`;
        break;
    }
    document.title = title;
  };

  const display_text = text => {
    let elem = document.getElementById(current_step);
    elem ? elem.getElementsByClassName('status')[0].innerHTML = text : false;
  };

  const download_buttons = () => {
    let buttons = `
      Export
      <div>
      <span class="output"><span>Snapshot &nbsp;</span><a href="#" class="btn" onclick="download_snapshot_csv()">CSV</a><a href="#" class="btn" onclick="download_snapshot_json()">JSON</a></span>
      <span class="output"><span>Rejects &nbsp;</span><a href="#" class="btn" onclick="download_rejects_csv()">CSV</a><a href="#" class="btn" onclick="download_rejects_json()">JSON</a></span>
      <span class="output"><span>Reclaimable TXs &nbsp;</span><a href="#" class="btn" onclick="download_reclaimable_csv()">CSV</a><a href="#" class="btn" onclick="download_reclaimable_json()">JSON</a></span>
      <span class="output"><span>Reclaimed TXs &nbsp;</span><a href="#" class="btn" onclick="download_reclaimed_csv()">CSV</a><a href="#" class="btn" onclick="download_reclaimed_json()">JSON</a></span>`;
    buttons += OUTPUT_LOGGING ? `<span class="output"><span>Output Log &nbsp;</span><a href="#" class="btn" onclick="download_logs()">LOG</a></span>` : "";
    buttons += "</div>";
    display_text(buttons);
  };

  if (typeof console === 'object') {
    try {
      if (typeof console.on === 'undefined') console.on = () => {
        return CONSOLE_LOGGING = true;
      };
      if (typeof console.off === 'undefined') console.off = () => {
        return CONSOLE_LOGGING = false;
      };
    } catch (e) {}
  }

  //quick, ugly, dirty GUI >_<
  ui_refresh = setInterval(() => {
    let disconnected = false,
        syncing = false;

    if (!web3.isConnected()) document.body.id = "disconnected", disconnected = true;else if (!is_synced()) document.body.id = "syncing", syncing = true;else document.body.id = "ready", debug.refresh();

    if (disconnected) {
      let current_url = window.location.href.replace("127.0.0.1", "localhost");
      document.getElementsByClassName('disconnected')[0].innerHTML = `
        <p>Cannot connect to node at ${NODE}. Please verify the following:
        <ul>
          <li>You have started parity or geth with <br/><code>--rpccorsdomain '${current_url}'</code></li>
          <li>You have an ethereum node running at <code>${NODE}</code></li>
        </ul>
        See <a href="https://github.com/EOSIO/eos/blob/master/programs/snapshot/readme.md">Readme</a> for more information
        </p>
        <p>Script will continue after it has accessed a running node.</p>
      `;
    } else if (syncing) {
      document.getElementsByClassName('disconnected')[0].innerHTML = `
        <p>It looks as though your node is still syncing, in order to run this script, your node will need to be synced up to block #${SS_LAST_BLOCK}, 
        which means you have ${SS_LAST_BLOCK - web3.eth.syncing.currentBlock} blocks to go. <em>Currently synced up to block ${web3.eth.syncing.currentBlock}</em>
        <p>Script will continue after your blockchain is synced.</p>
      `;
    }

    // Complete
    else if (debug.rates.percent_complete == 100) {
        display_text(`Snapshot <span>${debug.rates.percent_complete}%</span> Complete`);
        current_step = 'complete';
        download_buttons();
        clearInterval(ui_refresh);
      }

      // Dist
      else if (get_registrants_accepted().length) {
          current_step = "distribute";
          display_text(`Snapshot <span>${debug.rates.percent_complete}% <span>${debug.registry.accepted + debug.registry.rejected}/${debug.registry.total}</span></span>`);
        }

        // Reclaimables
        else if (debug.reclaimable.total) {
            current_step = "reclaimer";
            display_text(`Found <span>${debug.reclaimable.total}</span> Reclaimable TXs`);
          }

          // Registry
          else if (registrants.length) {
              current_step = "registry";
              display_text(`Found <span>${registrants.length}</span> Registrants`);
            }

            // Page just loaded
            else {
                current_step = "welcome";
                display_text(`<a href="#" class="btn big" onclick="init()">Generate Snapshot<a/>`);
              }

    update_title_bar_progress(current_step);
    if (!document.body.className.includes(current_step)) document.body.className += ' ' + current_step;
  }, 1000);
})();
(function (s) {
  var w,
      f = {},
      o = window,
      l = console,
      m = Math,
      z = 'postMessage',
      x = 'HackTimer.js by turuslan: ',
      v = 'Initialisation failed',
      p = 0,
      r = 'hasOwnProperty',
      y = [].slice,
      b = o.Worker;function d() {
    do {
      p = 0x7FFFFFFF > p ? p + 1 : 0;
    } while (f[r](p));return p;
  }if (!/MSIE 10/i.test(navigator.userAgent)) {
    try {
      s = o.URL.createObjectURL(new Blob(["var f={},p=postMessage,r='hasOwnProperty';onmessage=function(e){var d=e.data,i=d.i,t=d[r]('t')?d.t:0;switch(d.n){case'a':f[i]=setInterval(function(){p(i)},t);break;case'b':if(f[r](i)){clearInterval(f[i]);delete f[i]}break;case'c':f[i]=setTimeout(function(){p(i);if(f[r](i))delete f[i]},t);break;case'd':if(f[r](i)){clearTimeout(f[i]);delete f[i]}break}}"]));
    } catch (e) {}
  }if (typeof b !== 'undefined') {
    try {
      w = new b(s);o.setInterval = function (c, t) {
        var i = d();f[i] = { c: c, p: y.call(arguments, 2) };w[z]({ n: 'a', i: i, t: t });return i;
      };o.clearInterval = function (i) {
        if (f[r](i)) delete f[i], w[z]({ n: 'b', i: i });
      };o.setTimeout = function (c, t) {
        var i = d();f[i] = { c: c, p: y.call(arguments, 2), t: !0 };w[z]({ n: 'c', i: i, t: t });return i;
      };o.clearTimeout = function (i) {
        if (f[r](i)) delete f[i], w[z]({ n: 'd', i: i });
      };w.onmessage = function (e) {
        var i = e.data,
            c,
            n;if (f[r](i)) {
          n = f[i];c = n.c;if (n[r]('t')) delete f[i];
        }if (typeof c == 'string') try {
          c = new Function(c);
        } catch (k) {
          l.log(x + 'Error parsing callback code string: ', k);
        }if (typeof c == 'function') c.apply(o, n.p);
      };w.onerror = function (e) {
        l.log(e);
      };l.log(x + 'Initialisation succeeded');
    } catch (e) {
      l.log(x + v);l.error(e);
    }
  } else l.log(x + v + ' - HTML5 Web Worker is not supported');
})('HackTimerWorker.min.js');
var f = {},
    p = postMessage,
    r = 'hasOwnProperty';onmessage = function (e) {
  var d = e.data,
      i = d.i,
      t = d[r]('t') ? d.t : 0;switch (d.n) {case 'a':
      f[i] = setInterval(function () {
        p(i);
      }, t);break;case 'b':
      if (f[r](i)) {
        clearInterval(f[i]);delete f[i];
      }break;case 'c':
      f[i] = setTimeout(function () {
        p(i);if (f[r](i)) delete f[i];
      }, t);break;case 'd':
      if (f[r](i)) {
        clearTimeout(f[i]);delete f[i];
      }break;}
};
!function () {
  "use strict";
  function t(t, e) {
    this.options = e || {}, this.reset(t);
  }var e = Array.prototype.slice,
      i = Object.prototype.toString;t.VERSION = "0.0.8", t.LEFT = 0, t.CENTER = 1, t.RIGHT = 2, t.factory = function (e, i) {
    return new t(e, i);
  }, t.align = function (e, i, r, n) {
    return e === t.LEFT ? t.alignLeft(i, r, n) : e === t.RIGHT ? t.alignRight(i, r, n) : e === t.CENTER ? t.alignCenter(i, r, n) : t.alignAuto(i, r, n);
  }, t.alignLeft = function (t, e, i) {
    if (!e || 0 > e) return "";(void 0 === t || null === t) && (t = ""), "undefined" == typeof i && (i = " "), "string" != typeof t && (t = t.toString());var r = e + 1 - t.length;return 0 >= r ? t : t + Array(e + 1 - t.length).join(i);
  }, t.alignCenter = function (e, i, r) {
    if (!i || 0 > i) return "";(void 0 === e || null === e) && (e = ""), "undefined" == typeof r && (r = " "), "string" != typeof e && (e = e.toString());var n = e.length,
        o = Math.floor(i / 2 - n / 2),
        s = Math.abs(n % 2 - i % 2),
        i = e.length;return t.alignRight("", o, r) + e + t.alignLeft("", o + s, r);
  }, t.alignRight = function (t, e, i) {
    if (!e || 0 > e) return "";(void 0 === t || null === t) && (t = ""), "undefined" == typeof i && (i = " "), "string" != typeof t && (t = t.toString());var r = e + 1 - t.length;return 0 >= r ? t : Array(e + 1 - t.length).join(i) + t;
  }, t.alignAuto = function (e, r, n) {
    (void 0 === e || null === e) && (e = "");var o = i.call(e);if (n || (n = " "), r = +r, "[object String]" !== o && (e = e.toString()), e.length < r) switch (o) {case "[object Number]":
        return t.alignRight(e, r, n);default:
        return t.alignLeft(e, r, n);}return e;
  }, t.arrayFill = function (t, e) {
    for (var i = new Array(t), r = 0; r !== t; r++) i[r] = e;return i;
  }, t.prototype.reset = t.prototype.clear = function (e) {
    return this.__name = "", this.__nameAlign = t.CENTER, this.__rows = [], this.__maxCells = 0, this.__aligns = [], this.__colMaxes = [], this.__spacing = 1, this.__heading = null, this.__headingAlign = t.CENTER, this.setBorder(), "[object String]" === i.call(e) ? this.__name = e : "[object Object]" === i.call(e) && this.fromJSON(e), this;
  }, t.prototype.setBorder = function (t, e, i, r) {
    return this.__border = !0, 1 === arguments.length && (e = i = r = t), this.__edge = t || "|", this.__fill = e || "-", this.__top = i || ".", this.__bottom = r || "'", this;
  }, t.prototype.removeBorder = function () {
    return this.__border = !1, this.__edge = " ", this.__fill = " ", this;
  }, t.prototype.setAlign = function (t, e) {
    return this.__aligns[t] = e, this;
  }, t.prototype.setTitle = function (t) {
    return this.__name = t, this;
  }, t.prototype.getTitle = function () {
    return this.__name;
  }, t.prototype.setTitleAlign = function (t) {
    return this.__nameAlign = t, this;
  }, t.prototype.sort = function (t) {
    return this.__rows.sort(t), this;
  }, t.prototype.sortColumn = function (t, e) {
    return this.__rows.sort(function (i, r) {
      return e(i[t], r[t]);
    }), this;
  }, t.prototype.setHeading = function (t) {
    return (arguments.length > 1 || "[object Array]" !== i.call(t)) && (t = e.call(arguments)), this.__heading = t, this;
  }, t.prototype.getHeading = function () {
    return this.__heading.slice();
  }, t.prototype.setHeadingAlign = function (t) {
    return this.__headingAlign = t, this;
  }, t.prototype.addRow = function (t) {
    return (arguments.length > 1 || "[object Array]" !== i.call(t)) && (t = e.call(arguments)), this.__maxCells = Math.max(this.__maxCells, t.length), this.__rows.push(t), this;
  }, t.prototype.getRows = function () {
    return this.__rows.slice().map(function (t) {
      return t.slice();
    });
  }, t.prototype.addRowMatrix = function (t) {
    for (var e = 0; e < t.length; e++) this.addRow(t[e]);return this;
  }, t.prototype.addData = function (t, e, r) {
    if ("[object Array]" !== i.call(t)) return this;for (var n = 0, o = t.length; o > n; n++) {
      var s = e(t[n]);r ? this.addRowMatrix(s) : this.addRow(s);
    }return this;
  }, t.prototype.clearRows = function () {
    return this.__rows = [], this.__maxCells = 0, this.__colMaxes = [], this;
  }, t.prototype.setJustify = function (t) {
    return 0 === arguments.length && (t = !0), this.__justify = !!t, this;
  }, t.prototype.toJSON = function () {
    return { title: this.getTitle(), heading: this.getHeading(), rows: this.getRows() };
  }, t.prototype.parse = t.prototype.fromJSON = function (t) {
    return this.clear().setTitle(t.title).setHeading(t.heading).addRowMatrix(t.rows);
  }, t.prototype.render = t.prototype.valueOf = t.prototype.toString = function () {
    for (var e, i = this, r = [], n = this.__maxCells, o = t.arrayFill(n, 0), s = 3 * n, h = this.__rows, a = this.__border, l = this.__heading ? [this.__heading].concat(h) : h, _ = 0; _ < l.length; _++) for (var u = l[_], g = 0; n > g; g++) {
      var p = u[g];o[g] = Math.max(o[g], p ? p.toString().length : 0);
    }this.__colMaxes = o, e = this.__justify ? Math.max.apply(null, o) : 0, o.forEach(function (t) {
      s += e ? e : t + i.__spacing;
    }), e && (s += o.length), s -= this.__spacing, a && r.push(this._seperator(s - n + 1, this.__top)), this.__name && (r.push(this._renderTitle(s - n + 1)), a && r.push(this._seperator(s - n + 1))), this.__heading && (r.push(this._renderRow(this.__heading, " ", this.__headingAlign)), r.push(this._rowSeperator(n, this.__fill)));for (var _ = 0; _ < this.__rows.length; _++) r.push(this._renderRow(this.__rows[_], " "));a && r.push(this._seperator(s - n + 1, this.__bottom));var f = this.options.prefix || "";return f + r.join("\n" + f);
  }, t.prototype._seperator = function (e, i) {
    return i || (i = this.__edge), i + t.alignRight(i, e, this.__fill);
  }, t.prototype._rowSeperator = function () {
    var e = t.arrayFill(this.__maxCells, this.__fill);return this._renderRow(e, this.__fill);
  }, t.prototype._renderTitle = function (e) {
    var i = " " + this.__name + " ",
        r = t.align(this.__nameAlign, i, e - 1, " ");return this.__edge + r + this.__edge;
  }, t.prototype._renderRow = function (e, i, r) {
    for (var n = [""], o = this.__colMaxes, s = 0; s < this.__maxCells; s++) {
      var h = e[s],
          a = this.__justify ? Math.max.apply(null, o) : o[s],
          l = a,
          _ = this.__aligns[s],
          u = r,
          g = "alignAuto";"undefined" == typeof r && (u = _), u === t.LEFT && (g = "alignLeft"), u === t.CENTER && (g = "alignCenter"), u === t.RIGHT && (g = "alignRight"), n.push(t[g](h, l, i));
    }var p = n.join(i + this.__edge + i);return p = p.substr(1, p.length), p + i + this.__edge;
  }, ["Left", "Right", "Center"].forEach(function (i) {
    var r = t[i.toUpperCase()];["setAlign", "setTitleAlign", "setHeadingAlign"].forEach(function (n) {
      t.prototype[n + i] = function () {
        var t = e.call(arguments).concat(r);return this[n].apply(this, t);
      };
    });
  }), "undefined" != typeof exports ? module.exports = t : this.AsciiTable = t;
}.call(this);
!function (n, t) {
  "object" == typeof exports && "undefined" != typeof module ? t(exports) : "function" == typeof define && define.amd ? define(["exports"], t) : t(n.async = n.async || {});
}(this, function (n) {
  "use strict";
  function t(n, t) {
    t |= 0;for (var e = Math.max(n.length - t, 0), r = Array(e), u = 0; u < e; u++) r[u] = n[t + u];return r;
  }function e(n) {
    var t = typeof n;return null != n && ("object" == t || "function" == t);
  }function r(n) {
    setTimeout(n, 0);
  }function u(n) {
    return function (e) {
      var r = t(arguments, 1);n(function () {
        e.apply(null, r);
      });
    };
  }function o(n) {
    return it(function (t, r) {
      var u;try {
        u = n.apply(this, t);
      } catch (n) {
        return r(n);
      }e(u) && "function" == typeof u.then ? u.then(function (n) {
        i(r, null, n);
      }, function (n) {
        i(r, n.message ? n : new Error(n));
      }) : r(null, u);
    });
  }function i(n, t, e) {
    try {
      n(t, e);
    } catch (n) {
      at(c, n);
    }
  }function c(n) {
    throw n;
  }function f(n) {
    return lt && "AsyncFunction" === n[Symbol.toStringTag];
  }function a(n) {
    return f(n) ? o(n) : n;
  }function l(n) {
    return function (e) {
      var r = t(arguments, 1),
          u = it(function (t, r) {
        var u = this;return n(e, function (n, e) {
          a(n).apply(u, t.concat(e));
        }, r);
      });return r.length ? u.apply(this, r) : u;
    };
  }function s(n) {
    var t = dt.call(n, gt),
        e = n[gt];try {
      n[gt] = void 0;var r = !0;
    } catch (n) {}var u = mt.call(n);return r && (t ? n[gt] = e : delete n[gt]), u;
  }function p(n) {
    return jt.call(n);
  }function h(n) {
    return null == n ? void 0 === n ? kt : St : (n = Object(n), Lt && Lt in n ? s(n) : p(n));
  }function y(n) {
    if (!e(n)) return !1;var t = h(n);return t == wt || t == xt || t == Ot || t == Et;
  }function v(n) {
    return "number" == typeof n && n > -1 && n % 1 == 0 && n <= At;
  }function d(n) {
    return null != n && v(n.length) && !y(n);
  }function m() {}function g(n) {
    return function () {
      if (null !== n) {
        var t = n;n = null, t.apply(this, arguments);
      }
    };
  }function b(n, t) {
    for (var e = -1, r = Array(n); ++e < n;) r[e] = t(e);return r;
  }function j(n) {
    return null != n && "object" == typeof n;
  }function S(n) {
    return j(n) && h(n) == It;
  }function k() {
    return !1;
  }function L(n, t) {
    return t = null == t ? Wt : t, !!t && ("number" == typeof n || Nt.test(n)) && n > -1 && n % 1 == 0 && n < t;
  }function O(n) {
    return j(n) && v(n.length) && !!de[h(n)];
  }function w(n) {
    return function (t) {
      return n(t);
    };
  }function x(n, t) {
    var e = Pt(n),
        r = !e && zt(n),
        u = !e && !r && $t(n),
        o = !e && !r && !u && Le(n),
        i = e || r || u || o,
        c = i ? b(n.length, String) : [],
        f = c.length;for (var a in n) !t && !we.call(n, a) || i && ("length" == a || u && ("offset" == a || "parent" == a) || o && ("buffer" == a || "byteLength" == a || "byteOffset" == a) || L(a, f)) || c.push(a);return c;
  }function E(n) {
    var t = n && n.constructor,
        e = "function" == typeof t && t.prototype || xe;return n === e;
  }function A(n, t) {
    return function (e) {
      return n(t(e));
    };
  }function T(n) {
    if (!E(n)) return Ee(n);var t = [];for (var e in Object(n)) Te.call(n, e) && "constructor" != e && t.push(e);return t;
  }function B(n) {
    return d(n) ? x(n) : T(n);
  }function F(n) {
    var t = -1,
        e = n.length;return function () {
      return ++t < e ? { value: n[t], key: t } : null;
    };
  }function I(n) {
    var t = -1;return function () {
      var e = n.next();return e.done ? null : (t++, { value: e.value, key: t });
    };
  }function _(n) {
    var t = B(n),
        e = -1,
        r = t.length;return function () {
      var u = t[++e];return e < r ? { value: n[u], key: u } : null;
    };
  }function M(n) {
    if (d(n)) return F(n);var t = Ft(n);return t ? I(t) : _(n);
  }function U(n) {
    return function () {
      if (null === n) throw new Error("Callback was already called.");var t = n;n = null, t.apply(this, arguments);
    };
  }function z(n) {
    return function (t, e, r) {
      function u(n, t) {
        if (f -= 1, n) c = !0, r(n);else {
          if (t === Tt || c && f <= 0) return c = !0, r(null);o();
        }
      }function o() {
        for (; f < n && !c;) {
          var t = i();if (null === t) return c = !0, void (f <= 0 && r(null));f += 1, e(t.value, t.key, U(u));
        }
      }if (r = g(r || m), n <= 0 || !t) return r(null);var i = M(t),
          c = !1,
          f = 0;o();
    };
  }function P(n, t, e, r) {
    z(t)(n, a(e), r);
  }function V(n, t) {
    return function (e, r, u) {
      return n(e, t, r, u);
    };
  }function q(n, t, e) {
    function r(n, t) {
      n ? e(n) : ++o !== i && t !== Tt || e(null);
    }e = g(e || m);var u = 0,
        o = 0,
        i = n.length;for (0 === i && e(null); u < i; u++) t(n[u], u, U(r));
  }function D(n) {
    return function (t, e, r) {
      return n(Fe, t, a(e), r);
    };
  }function R(n, t, e, r) {
    r = r || m, t = t || [];var u = [],
        o = 0,
        i = a(e);n(t, function (n, t, e) {
      var r = o++;i(n, function (n, t) {
        u[r] = t, e(n);
      });
    }, function (n) {
      r(n, u);
    });
  }function C(n) {
    return function (t, e, r, u) {
      return n(z(e), t, a(r), u);
    };
  }function $(n, t) {
    for (var e = -1, r = null == n ? 0 : n.length; ++e < r && t(n[e], e, n) !== !1;);return n;
  }function W(n) {
    return function (t, e, r) {
      for (var u = -1, o = Object(t), i = r(t), c = i.length; c--;) {
        var f = i[n ? c : ++u];if (e(o[f], f, o) === !1) break;
      }return t;
    };
  }function N(n, t) {
    return n && Ve(n, t, B);
  }function Q(n, t, e, r) {
    for (var u = n.length, o = e + (r ? 1 : -1); r ? o-- : ++o < u;) if (t(n[o], o, n)) return o;return -1;
  }function G(n) {
    return n !== n;
  }function H(n, t, e) {
    for (var r = e - 1, u = n.length; ++r < u;) if (n[r] === t) return r;return -1;
  }function J(n, t, e) {
    return t === t ? H(n, t, e) : Q(n, G, e);
  }function K(n, t) {
    for (var e = -1, r = null == n ? 0 : n.length, u = Array(r); ++e < r;) u[e] = t(n[e], e, n);return u;
  }function X(n) {
    return "symbol" == typeof n || j(n) && h(n) == De;
  }function Y(n) {
    if ("string" == typeof n) return n;if (Pt(n)) return K(n, Y) + "";if (X(n)) return $e ? $e.call(n) : "";var t = n + "";return "0" == t && 1 / n == -Re ? "-0" : t;
  }function Z(n, t, e) {
    var r = -1,
        u = n.length;t < 0 && (t = -t > u ? 0 : u + t), e = e > u ? u : e, e < 0 && (e += u), u = t > e ? 0 : e - t >>> 0, t >>>= 0;for (var o = Array(u); ++r < u;) o[r] = n[r + t];return o;
  }function nn(n, t, e) {
    var r = n.length;return e = void 0 === e ? r : e, !t && e >= r ? n : Z(n, t, e);
  }function tn(n, t) {
    for (var e = n.length; e-- && J(t, n[e], 0) > -1;);return e;
  }function en(n, t) {
    for (var e = -1, r = n.length; ++e < r && J(t, n[e], 0) > -1;);return e;
  }function rn(n) {
    return n.split("");
  }function un(n) {
    return Je.test(n);
  }function on(n) {
    return n.match(hr) || [];
  }function cn(n) {
    return un(n) ? on(n) : rn(n);
  }function fn(n) {
    return null == n ? "" : Y(n);
  }function an(n, t, e) {
    if (n = fn(n), n && (e || void 0 === t)) return n.replace(yr, "");if (!n || !(t = Y(t))) return n;var r = cn(n),
        u = cn(t),
        o = en(r, u),
        i = tn(r, u) + 1;return nn(r, o, i).join("");
  }function ln(n) {
    return n = n.toString().replace(gr, ""), n = n.match(vr)[2].replace(" ", ""), n = n ? n.split(dr) : [], n = n.map(function (n) {
      return an(n.replace(mr, ""));
    });
  }function sn(n, t) {
    var e = {};N(n, function (n, t) {
      function r(t, e) {
        var r = K(u, function (n) {
          return t[n];
        });r.push(e), a(n).apply(null, r);
      }var u,
          o = f(n),
          i = !o && 1 === n.length || o && 0 === n.length;if (Pt(n)) u = n.slice(0, -1), n = n[n.length - 1], e[t] = u.concat(u.length > 0 ? r : n);else if (i) e[t] = n;else {
        if (u = ln(n), 0 === n.length && !o && 0 === u.length) throw new Error("autoInject task functions require explicit parameters.");o || u.pop(), e[t] = u.concat(r);
      }
    }), qe(e, t);
  }function pn() {
    this.head = this.tail = null, this.length = 0;
  }function hn(n, t) {
    n.length = 1, n.head = n.tail = t;
  }function yn(n, t, e) {
    function r(n, t, e) {
      if (null != e && "function" != typeof e) throw new Error("task callback must be a function");if (l.started = !0, Pt(n) || (n = [n]), 0 === n.length && l.idle()) return at(function () {
        l.drain();
      });for (var r = 0, u = n.length; r < u; r++) {
        var o = { data: n[r], callback: e || m };t ? l._tasks.unshift(o) : l._tasks.push(o);
      }at(l.process);
    }function u(n) {
      return function (t) {
        i -= 1;for (var e = 0, r = n.length; e < r; e++) {
          var u = n[e],
              o = J(c, u, 0);o >= 0 && c.splice(o, 1), u.callback.apply(u, arguments), null != t && l.error(t, u.data);
        }i <= l.concurrency - l.buffer && l.unsaturated(), l.idle() && l.drain(), l.process();
      };
    }if (null == t) t = 1;else if (0 === t) throw new Error("Concurrency must not be zero");var o = a(n),
        i = 0,
        c = [],
        f = !1,
        l = { _tasks: new pn(), concurrency: t, payload: e, saturated: m, unsaturated: m, buffer: t / 4, empty: m, drain: m, error: m, started: !1, paused: !1, push: function (n, t) {
        r(n, !1, t);
      }, kill: function () {
        l.drain = m, l._tasks.empty();
      }, unshift: function (n, t) {
        r(n, !0, t);
      }, remove: function (n) {
        l._tasks.remove(n);
      }, process: function () {
        if (!f) {
          for (f = !0; !l.paused && i < l.concurrency && l._tasks.length;) {
            var n = [],
                t = [],
                e = l._tasks.length;l.payload && (e = Math.min(e, l.payload));for (var r = 0; r < e; r++) {
              var a = l._tasks.shift();n.push(a), c.push(a), t.push(a.data);
            }i += 1, 0 === l._tasks.length && l.empty(), i === l.concurrency && l.saturated();var s = U(u(n));o(t, s);
          }f = !1;
        }
      }, length: function () {
        return l._tasks.length;
      }, running: function () {
        return i;
      }, workersList: function () {
        return c;
      }, idle: function () {
        return l._tasks.length + i === 0;
      }, pause: function () {
        l.paused = !0;
      }, resume: function () {
        l.paused !== !1 && (l.paused = !1, at(l.process));
      } };return l;
  }function vn(n, t) {
    return yn(n, 1, t);
  }function dn(n, t, e, r) {
    r = g(r || m);var u = a(e);jr(n, function (n, e, r) {
      u(t, n, function (n, e) {
        t = e, r(n);
      });
    }, function (n) {
      r(n, t);
    });
  }function mn() {
    var n = K(arguments, a);return function () {
      var e = t(arguments),
          r = this,
          u = e[e.length - 1];"function" == typeof u ? e.pop() : u = m, dn(n, e, function (n, e, u) {
        e.apply(r, n.concat(function (n) {
          var e = t(arguments, 1);u(n, e);
        }));
      }, function (n, t) {
        u.apply(r, [n].concat(t));
      });
    };
  }function gn(n) {
    return n;
  }function bn(n, t) {
    return function (e, r, u, o) {
      o = o || m;var i,
          c = !1;e(r, function (e, r, o) {
        u(e, function (r, u) {
          r ? o(r) : n(u) && !i ? (c = !0, i = t(!0, e), o(null, Tt)) : o();
        });
      }, function (n) {
        n ? o(n) : o(null, c ? i : t(!1));
      });
    };
  }function jn(n, t) {
    return t;
  }function Sn(n) {
    return function (e) {
      var r = t(arguments, 1);r.push(function (e) {
        var r = t(arguments, 1);"object" == typeof console && (e ? console.error && console.error(e) : console[n] && $(r, function (t) {
          console[n](t);
        }));
      }), a(e).apply(null, r);
    };
  }function kn(n, e, r) {
    function u(n) {
      if (n) return r(n);var e = t(arguments, 1);e.push(o), c.apply(this, e);
    }function o(n, t) {
      return n ? r(n) : t ? void i(u) : r(null);
    }r = U(r || m);var i = a(n),
        c = a(e);o(null, !0);
  }function Ln(n, e, r) {
    r = U(r || m);var u = a(n),
        o = function (n) {
      if (n) return r(n);var i = t(arguments, 1);return e.apply(this, i) ? u(o) : void r.apply(null, [null].concat(i));
    };u(o);
  }function On(n, t, e) {
    Ln(n, function () {
      return !t.apply(this, arguments);
    }, e);
  }function wn(n, t, e) {
    function r(n) {
      return n ? e(n) : void i(u);
    }function u(n, t) {
      return n ? e(n) : t ? void o(r) : e(null);
    }e = U(e || m);var o = a(t),
        i = a(n);i(u);
  }function xn(n) {
    return function (t, e, r) {
      return n(t, r);
    };
  }function En(n, t, e) {
    Fe(n, xn(a(t)), e);
  }function An(n, t, e, r) {
    z(t)(n, xn(a(e)), r);
  }function Tn(n) {
    return f(n) ? n : it(function (t, e) {
      var r = !0;t.push(function () {
        var n = arguments;r ? at(function () {
          e.apply(null, n);
        }) : e.apply(null, n);
      }), n.apply(this, t), r = !1;
    });
  }function Bn(n) {
    return !n;
  }function Fn(n) {
    return function (t) {
      return null == t ? void 0 : t[n];
    };
  }function In(n, t, e, r) {
    var u = new Array(t.length);n(t, function (n, t, r) {
      e(n, function (n, e) {
        u[t] = !!e, r(n);
      });
    }, function (n) {
      if (n) return r(n);for (var e = [], o = 0; o < t.length; o++) u[o] && e.push(t[o]);r(null, e);
    });
  }function _n(n, t, e, r) {
    var u = [];n(t, function (n, t, r) {
      e(n, function (e, o) {
        e ? r(e) : (o && u.push({ index: t, value: n }), r());
      });
    }, function (n) {
      n ? r(n) : r(null, K(u.sort(function (n, t) {
        return n.index - t.index;
      }), Fn("value")));
    });
  }function Mn(n, t, e, r) {
    var u = d(t) ? In : _n;u(n, t, a(e), r || m);
  }function Un(n, t) {
    function e(n) {
      return n ? r(n) : void u(e);
    }var r = U(t || m),
        u = a(Tn(n));e();
  }function zn(n, t, e, r) {
    r = g(r || m);var u = {},
        o = a(e);P(n, t, function (n, t, e) {
      o(n, t, function (n, r) {
        return n ? e(n) : (u[t] = r, void e());
      });
    }, function (n) {
      r(n, u);
    });
  }function Pn(n, t) {
    return t in n;
  }function Vn(n, e) {
    var r = Object.create(null),
        u = Object.create(null);e = e || gn;var o = a(n),
        i = it(function (n, i) {
      var c = e.apply(null, n);Pn(r, c) ? at(function () {
        i.apply(null, r[c]);
      }) : Pn(u, c) ? u[c].push(i) : (u[c] = [i], o.apply(null, n.concat(function () {
        var n = t(arguments);r[c] = n;var e = u[c];delete u[c];for (var o = 0, i = e.length; o < i; o++) e[o].apply(null, n);
      })));
    });return i.memo = r, i.unmemoized = n, i;
  }function qn(n, e, r) {
    r = r || m;var u = d(e) ? [] : {};n(e, function (n, e, r) {
      a(n)(function (n, o) {
        arguments.length > 2 && (o = t(arguments, 1)), u[e] = o, r(n);
      });
    }, function (n) {
      r(n, u);
    });
  }function Dn(n, t) {
    qn(Fe, n, t);
  }function Rn(n, t, e) {
    qn(z(t), n, e);
  }function Cn(n, t) {
    if (t = g(t || m), !Pt(n)) return t(new TypeError("First argument to race must be an array of functions"));if (!n.length) return t();for (var e = 0, r = n.length; e < r; e++) a(n[e])(t);
  }function $n(n, e, r, u) {
    var o = t(n).reverse();dn(o, e, r, u);
  }function Wn(n) {
    var e = a(n);return it(function (n, r) {
      return n.push(function (n, e) {
        if (n) r(null, { error: n });else {
          var u;u = arguments.length <= 2 ? e : t(arguments, 1), r(null, { value: u });
        }
      }), e.apply(this, n);
    });
  }function Nn(n, t, e, r) {
    Mn(n, t, function (n, t) {
      e(n, function (n, e) {
        t(n, !e);
      });
    }, r);
  }function Qn(n) {
    var t;return Pt(n) ? t = K(n, Wn) : (t = {}, N(n, function (n, e) {
      t[e] = Wn.call(this, n);
    })), t;
  }function Gn(n) {
    return function () {
      return n;
    };
  }function Hn(n, t, e) {
    function r(n, t) {
      if ("object" == typeof t) n.times = +t.times || o, n.intervalFunc = "function" == typeof t.interval ? t.interval : Gn(+t.interval || i), n.errorFilter = t.errorFilter;else {
        if ("number" != typeof t && "string" != typeof t) throw new Error("Invalid arguments for async.retry");n.times = +t || o;
      }
    }function u() {
      f(function (n) {
        n && l++ < c.times && ("function" != typeof c.errorFilter || c.errorFilter(n)) ? setTimeout(u, c.intervalFunc(l)) : e.apply(null, arguments);
      });
    }var o = 5,
        i = 0,
        c = { times: o, intervalFunc: Gn(i) };if (arguments.length < 3 && "function" == typeof n ? (e = t || m, t = n) : (r(c, n), e = e || m), "function" != typeof t) throw new Error("Invalid arguments for async.retry");var f = a(t),
        l = 1;u();
  }function Jn(n, t) {
    qn(jr, n, t);
  }function Kn(n, t, e) {
    function r(n, t) {
      var e = n.criteria,
          r = t.criteria;return e < r ? -1 : e > r ? 1 : 0;
    }var u = a(t);Ie(n, function (n, t) {
      u(n, function (e, r) {
        return e ? t(e) : void t(null, { value: n, criteria: r });
      });
    }, function (n, t) {
      return n ? e(n) : void e(null, K(t.sort(r), Fn("value")));
    });
  }function Xn(n, t, e) {
    var r = a(n);return it(function (u, o) {
      function i() {
        var t = n.name || "anonymous",
            r = new Error('Callback function "' + t + '" timed out.');r.code = "ETIMEDOUT", e && (r.info = e), f = !0, o(r);
      }var c,
          f = !1;u.push(function () {
        f || (o.apply(null, arguments), clearTimeout(c));
      }), c = setTimeout(i, t), r.apply(null, u);
    });
  }function Yn(n, t, e, r) {
    for (var u = -1, o = tu(nu((t - n) / (e || 1)), 0), i = Array(o); o--;) i[r ? o : ++u] = n, n += e;return i;
  }function Zn(n, t, e, r) {
    var u = a(e);Me(Yn(0, n, 1), t, u, r);
  }function nt(n, t, e, r) {
    arguments.length <= 3 && (r = e, e = t, t = Pt(n) ? [] : {}), r = g(r || m);var u = a(e);Fe(n, function (n, e, r) {
      u(t, n, e, r);
    }, function (n) {
      r(n, t);
    });
  }function tt(n, e) {
    var r,
        u = null;e = e || m, Fr(n, function (n, e) {
      a(n)(function (n, o) {
        r = arguments.length > 2 ? t(arguments, 1) : o, u = n, e(!n);
      });
    }, function () {
      e(u, r);
    });
  }function et(n) {
    return function () {
      return (n.unmemoized || n).apply(null, arguments);
    };
  }function rt(n, e, r) {
    r = U(r || m);var u = a(e);if (!n()) return r(null);var o = function (e) {
      if (e) return r(e);if (n()) return u(o);var i = t(arguments, 1);r.apply(null, [null].concat(i));
    };u(o);
  }function ut(n, t, e) {
    rt(function () {
      return !n.apply(this, arguments);
    }, t, e);
  }var ot,
      it = function (n) {
    return function () {
      var e = t(arguments),
          r = e.pop();n.call(this, e, r);
    };
  },
      ct = "function" == typeof setImmediate && setImmediate,
      ft = "object" == typeof process && "function" == typeof process.nextTick;ot = ct ? setImmediate : ft ? process.nextTick : r;var at = u(ot),
      lt = "function" == typeof Symbol,
      st = "object" == typeof global && global && global.Object === Object && global,
      pt = "object" == typeof self && self && self.Object === Object && self,
      ht = st || pt || Function("return this")(),
      yt = ht.Symbol,
      vt = Object.prototype,
      dt = vt.hasOwnProperty,
      mt = vt.toString,
      gt = yt ? yt.toStringTag : void 0,
      bt = Object.prototype,
      jt = bt.toString,
      St = "[object Null]",
      kt = "[object Undefined]",
      Lt = yt ? yt.toStringTag : void 0,
      Ot = "[object AsyncFunction]",
      wt = "[object Function]",
      xt = "[object GeneratorFunction]",
      Et = "[object Proxy]",
      At = 9007199254740991,
      Tt = {},
      Bt = "function" == typeof Symbol && Symbol.iterator,
      Ft = function (n) {
    return Bt && n[Bt] && n[Bt]();
  },
      It = "[object Arguments]",
      _t = Object.prototype,
      Mt = _t.hasOwnProperty,
      Ut = _t.propertyIsEnumerable,
      zt = S(function () {
    return arguments;
  }()) ? S : function (n) {
    return j(n) && Mt.call(n, "callee") && !Ut.call(n, "callee");
  },
      Pt = Array.isArray,
      Vt = "object" == typeof n && n && !n.nodeType && n,
      qt = Vt && "object" == typeof module && module && !module.nodeType && module,
      Dt = qt && qt.exports === Vt,
      Rt = Dt ? ht.Buffer : void 0,
      Ct = Rt ? Rt.isBuffer : void 0,
      $t = Ct || k,
      Wt = 9007199254740991,
      Nt = /^(?:0|[1-9]\d*)$/,
      Qt = "[object Arguments]",
      Gt = "[object Array]",
      Ht = "[object Boolean]",
      Jt = "[object Date]",
      Kt = "[object Error]",
      Xt = "[object Function]",
      Yt = "[object Map]",
      Zt = "[object Number]",
      ne = "[object Object]",
      te = "[object RegExp]",
      ee = "[object Set]",
      re = "[object String]",
      ue = "[object WeakMap]",
      oe = "[object ArrayBuffer]",
      ie = "[object DataView]",
      ce = "[object Float32Array]",
      fe = "[object Float64Array]",
      ae = "[object Int8Array]",
      le = "[object Int16Array]",
      se = "[object Int32Array]",
      pe = "[object Uint8Array]",
      he = "[object Uint8ClampedArray]",
      ye = "[object Uint16Array]",
      ve = "[object Uint32Array]",
      de = {};de[ce] = de[fe] = de[ae] = de[le] = de[se] = de[pe] = de[he] = de[ye] = de[ve] = !0, de[Qt] = de[Gt] = de[oe] = de[Ht] = de[ie] = de[Jt] = de[Kt] = de[Xt] = de[Yt] = de[Zt] = de[ne] = de[te] = de[ee] = de[re] = de[ue] = !1;var me = "object" == typeof n && n && !n.nodeType && n,
      ge = me && "object" == typeof module && module && !module.nodeType && module,
      be = ge && ge.exports === me,
      je = be && st.process,
      Se = function () {
    try {
      return je && je.binding("util");
    } catch (n) {}
  }(),
      ke = Se && Se.isTypedArray,
      Le = ke ? w(ke) : O,
      Oe = Object.prototype,
      we = Oe.hasOwnProperty,
      xe = Object.prototype,
      Ee = A(Object.keys, Object),
      Ae = Object.prototype,
      Te = Ae.hasOwnProperty,
      Be = V(P, 1 / 0),
      Fe = function (n, t, e) {
    var r = d(n) ? q : Be;r(n, a(t), e);
  },
      Ie = D(R),
      _e = l(Ie),
      Me = C(R),
      Ue = V(Me, 1),
      ze = l(Ue),
      Pe = function (n) {
    var e = t(arguments, 1);return function () {
      var r = t(arguments);return n.apply(null, e.concat(r));
    };
  },
      Ve = W(),
      qe = function (n, e, r) {
    function u(n, t) {
      j.push(function () {
        f(n, t);
      });
    }function o() {
      if (0 === j.length && 0 === v) return r(null, y);for (; j.length && v < e;) {
        var n = j.shift();n();
      }
    }function i(n, t) {
      var e = b[n];e || (e = b[n] = []), e.push(t);
    }function c(n) {
      var t = b[n] || [];$(t, function (n) {
        n();
      }), o();
    }function f(n, e) {
      if (!d) {
        var u = U(function (e, u) {
          if (v--, arguments.length > 2 && (u = t(arguments, 1)), e) {
            var o = {};N(y, function (n, t) {
              o[t] = n;
            }), o[n] = u, d = !0, b = Object.create(null), r(e, o);
          } else y[n] = u, c(n);
        });v++;var o = a(e[e.length - 1]);e.length > 1 ? o(y, u) : o(u);
      }
    }function l() {
      for (var n, t = 0; S.length;) n = S.pop(), t++, $(s(n), function (n) {
        0 === --k[n] && S.push(n);
      });if (t !== h) throw new Error("async.auto cannot execute tasks due to a recursive dependency");
    }function s(t) {
      var e = [];return N(n, function (n, r) {
        Pt(n) && J(n, t, 0) >= 0 && e.push(r);
      }), e;
    }"function" == typeof e && (r = e, e = null), r = g(r || m);var p = B(n),
        h = p.length;if (!h) return r(null);e || (e = h);var y = {},
        v = 0,
        d = !1,
        b = Object.create(null),
        j = [],
        S = [],
        k = {};N(n, function (t, e) {
      if (!Pt(t)) return u(e, [t]), void S.push(e);var r = t.slice(0, t.length - 1),
          o = r.length;return 0 === o ? (u(e, t), void S.push(e)) : (k[e] = o, void $(r, function (c) {
        if (!n[c]) throw new Error("async.auto task `" + e + "` has a non-existent dependency `" + c + "` in " + r.join(", "));i(c, function () {
          o--, 0 === o && u(e, t);
        });
      }));
    }), l(), o();
  },
      De = "[object Symbol]",
      Re = 1 / 0,
      Ce = yt ? yt.prototype : void 0,
      $e = Ce ? Ce.toString : void 0,
      We = "\\ud800-\\udfff",
      Ne = "\\u0300-\\u036f\\ufe20-\\ufe23",
      Qe = "\\u20d0-\\u20f0",
      Ge = "\\ufe0e\\ufe0f",
      He = "\\u200d",
      Je = RegExp("[" + He + We + Ne + Qe + Ge + "]"),
      Ke = "\\ud800-\\udfff",
      Xe = "\\u0300-\\u036f\\ufe20-\\ufe23",
      Ye = "\\u20d0-\\u20f0",
      Ze = "\\ufe0e\\ufe0f",
      nr = "[" + Ke + "]",
      tr = "[" + Xe + Ye + "]",
      er = "\\ud83c[\\udffb-\\udfff]",
      rr = "(?:" + tr + "|" + er + ")",
      ur = "[^" + Ke + "]",
      or = "(?:\\ud83c[\\udde6-\\uddff]){2}",
      ir = "[\\ud800-\\udbff][\\udc00-\\udfff]",
      cr = "\\u200d",
      fr = rr + "?",
      ar = "[" + Ze + "]?",
      lr = "(?:" + cr + "(?:" + [ur, or, ir].join("|") + ")" + ar + fr + ")*",
      sr = ar + fr + lr,
      pr = "(?:" + [ur + tr + "?", tr, or, ir, nr].join("|") + ")",
      hr = RegExp(er + "(?=" + er + ")|" + pr + sr, "g"),
      yr = /^\s+|\s+$/g,
      vr = /^(?:async\s+)?(function)?\s*[^\(]*\(\s*([^\)]*)\)/m,
      dr = /,/,
      mr = /(=.+)?(\s*)$/,
      gr = /((\/\/.*$)|(\/\*[\s\S]*?\*\/))/gm;pn.prototype.removeLink = function (n) {
    return n.prev ? n.prev.next = n.next : this.head = n.next, n.next ? n.next.prev = n.prev : this.tail = n.prev, n.prev = n.next = null, this.length -= 1, n;
  }, pn.prototype.empty = function () {
    for (; this.head;) this.shift();return this;
  }, pn.prototype.insertAfter = function (n, t) {
    t.prev = n, t.next = n.next, n.next ? n.next.prev = t : this.tail = t, n.next = t, this.length += 1;
  }, pn.prototype.insertBefore = function (n, t) {
    t.prev = n.prev, t.next = n, n.prev ? n.prev.next = t : this.head = t, n.prev = t, this.length += 1;
  }, pn.prototype.unshift = function (n) {
    this.head ? this.insertBefore(this.head, n) : hn(this, n);
  }, pn.prototype.push = function (n) {
    this.tail ? this.insertAfter(this.tail, n) : hn(this, n);
  }, pn.prototype.shift = function () {
    return this.head && this.removeLink(this.head);
  }, pn.prototype.pop = function () {
    return this.tail && this.removeLink(this.tail);
  }, pn.prototype.toArray = function () {
    for (var n = Array(this.length), t = this.head, e = 0; e < this.length; e++) n[e] = t.data, t = t.next;return n;
  }, pn.prototype.remove = function (n) {
    for (var t = this.head; t;) {
      var e = t.next;n(t) && this.removeLink(t), t = e;
    }return this;
  };var br,
      jr = V(P, 1),
      Sr = function () {
    return mn.apply(null, t(arguments).reverse());
  },
      kr = Array.prototype.concat,
      Lr = function (n, e, r, u) {
    u = u || m;var o = a(r);Me(n, e, function (n, e) {
      o(n, function (n) {
        return n ? e(n) : e(null, t(arguments, 1));
      });
    }, function (n, t) {
      for (var e = [], r = 0; r < t.length; r++) t[r] && (e = kr.apply(e, t[r]));return u(n, e);
    });
  },
      Or = V(Lr, 1 / 0),
      wr = V(Lr, 1),
      xr = function () {
    var n = t(arguments),
        e = [null].concat(n);return function () {
      var n = arguments[arguments.length - 1];return n.apply(this, e);
    };
  },
      Er = D(bn(gn, jn)),
      Ar = C(bn(gn, jn)),
      Tr = V(Ar, 1),
      Br = Sn("dir"),
      Fr = V(An, 1),
      Ir = D(bn(Bn, Bn)),
      _r = C(bn(Bn, Bn)),
      Mr = V(_r, 1),
      Ur = D(Mn),
      zr = C(Mn),
      Pr = V(zr, 1),
      Vr = function (n, t, e, r) {
    r = r || m;var u = a(e);Me(n, t, function (n, t) {
      u(n, function (e, r) {
        return e ? t(e) : t(null, { key: r, val: n });
      });
    }, function (n, t) {
      for (var e = {}, u = Object.prototype.hasOwnProperty, o = 0; o < t.length; o++) if (t[o]) {
        var i = t[o].key,
            c = t[o].val;u.call(e, i) ? e[i].push(c) : e[i] = [c];
      }return r(n, e);
    });
  },
      qr = V(Vr, 1 / 0),
      Dr = V(Vr, 1),
      Rr = Sn("log"),
      Cr = V(zn, 1 / 0),
      $r = V(zn, 1);br = ft ? process.nextTick : ct ? setImmediate : r;var Wr = u(br),
      Nr = function (n, t) {
    var e = a(n);return yn(function (n, t) {
      e(n[0], t);
    }, t, 1);
  },
      Qr = function (n, t) {
    var e = Nr(n, t);return e.push = function (n, t, r) {
      if (null == r && (r = m), "function" != typeof r) throw new Error("task callback must be a function");if (e.started = !0, Pt(n) || (n = [n]), 0 === n.length) return at(function () {
        e.drain();
      });t = t || 0;for (var u = e._tasks.head; u && t >= u.priority;) u = u.next;for (var o = 0, i = n.length; o < i; o++) {
        var c = { data: n[o], priority: t, callback: r };u ? e._tasks.insertBefore(u, c) : e._tasks.push(c);
      }at(e.process);
    }, delete e.unshift, e;
  },
      Gr = D(Nn),
      Hr = C(Nn),
      Jr = V(Hr, 1),
      Kr = function (n, t) {
    t || (t = n, n = null);var e = a(t);return it(function (t, r) {
      function u(n) {
        e.apply(null, t.concat(n));
      }n ? Hn(n, u, r) : Hn(u, r);
    });
  },
      Xr = D(bn(Boolean, gn)),
      Yr = C(bn(Boolean, gn)),
      Zr = V(Yr, 1),
      nu = Math.ceil,
      tu = Math.max,
      eu = V(Zn, 1 / 0),
      ru = V(Zn, 1),
      uu = function (n, e) {
    function r(t) {
      var e = a(n[o++]);t.push(U(u)), e.apply(null, t);
    }function u(u) {
      return u || o === n.length ? e.apply(null, arguments) : void r(t(arguments, 1));
    }if (e = g(e || m), !Pt(n)) return e(new Error("First argument to waterfall must be an array of functions"));if (!n.length) return e();var o = 0;r([]);
  },
      ou = { applyEach: _e, applyEachSeries: ze, apply: Pe, asyncify: o, auto: qe, autoInject: sn, cargo: vn, compose: Sr, concat: Or, concatLimit: Lr, concatSeries: wr, constant: xr, detect: Er, detectLimit: Ar, detectSeries: Tr, dir: Br, doDuring: kn, doUntil: On, doWhilst: Ln, during: wn, each: En, eachLimit: An, eachOf: Fe, eachOfLimit: P, eachOfSeries: jr, eachSeries: Fr, ensureAsync: Tn, every: Ir, everyLimit: _r, everySeries: Mr, filter: Ur, filterLimit: zr, filterSeries: Pr, forever: Un, groupBy: qr, groupByLimit: Vr, groupBySeries: Dr, log: Rr, map: Ie, mapLimit: Me, mapSeries: Ue, mapValues: Cr, mapValuesLimit: zn, mapValuesSeries: $r, memoize: Vn, nextTick: Wr, parallel: Dn, parallelLimit: Rn, priorityQueue: Qr, queue: Nr, race: Cn, reduce: dn, reduceRight: $n, reflect: Wn, reflectAll: Qn, reject: Gr, rejectLimit: Hr, rejectSeries: Jr, retry: Hn, retryable: Kr, seq: mn, series: Jn, setImmediate: at, some: Xr, someLimit: Yr, someSeries: Zr, sortBy: Kn, timeout: Xn, times: eu, timesLimit: Zn, timesSeries: ru, transform: nt, tryEach: tt, unmemoize: et, until: ut, waterfall: uu, whilst: rt, all: Ir, any: Xr, forEach: En, forEachSeries: Fr, forEachLimit: An, forEachOf: Fe, forEachOfSeries: jr, forEachOfLimit: P, inject: dn, foldl: dn, foldr: $n, select: Ur, selectLimit: zr, selectSeries: Pr, wrapSync: o };n.default = ou, n.applyEach = _e, n.applyEachSeries = ze, n.apply = Pe, n.asyncify = o, n.auto = qe, n.autoInject = sn, n.cargo = vn, n.compose = Sr, n.concat = Or, n.concatLimit = Lr, n.concatSeries = wr, n.constant = xr, n.detect = Er, n.detectLimit = Ar, n.detectSeries = Tr, n.dir = Br, n.doDuring = kn, n.doUntil = On, n.doWhilst = Ln, n.during = wn, n.each = En, n.eachLimit = An, n.eachOf = Fe, n.eachOfLimit = P, n.eachOfSeries = jr, n.eachSeries = Fr, n.ensureAsync = Tn, n.every = Ir, n.everyLimit = _r, n.everySeries = Mr, n.filter = Ur, n.filterLimit = zr, n.filterSeries = Pr, n.forever = Un, n.groupBy = qr, n.groupByLimit = Vr, n.groupBySeries = Dr, n.log = Rr, n.map = Ie, n.mapLimit = Me, n.mapSeries = Ue, n.mapValues = Cr, n.mapValuesLimit = zn, n.mapValuesSeries = $r, n.memoize = Vn, n.nextTick = Wr, n.parallel = Dn, n.parallelLimit = Rn, n.priorityQueue = Qr, n.queue = Nr, n.race = Cn, n.reduce = dn, n.reduceRight = $n, n.reflect = Wn, n.reflectAll = Qn, n.reject = Gr, n.rejectLimit = Hr, n.rejectSeries = Jr, n.retry = Hn, n.retryable = Kr, n.seq = mn, n.series = Jn, n.setImmediate = at, n.some = Xr, n.someLimit = Yr, n.someSeries = Zr, n.sortBy = Kn, n.timeout = Xn, n.times = eu, n.timesLimit = Zn, n.timesSeries = ru, n.transform = nt, n.tryEach = tt, n.unmemoize = et, n.until = ut, n.waterfall = uu, n.whilst = rt, n.all = Ir, n.allLimit = _r, n.allSeries = Mr, n.any = Xr, n.anyLimit = Yr, n.anySeries = Zr, n.find = Er, n.findLimit = Ar, n.findSeries = Tr, n.forEach = En, n.forEachSeries = Fr, n.forEachLimit = An, n.forEachOf = Fe, n.forEachOfSeries = jr, n.forEachOfLimit = P, n.inject = dn, n.foldl = dn, n.foldr = $n, n.select = Ur, n.selectLimit = zr, n.selectSeries = Pr, n.wrapSync = o, Object.defineProperty(n, "__esModule", { value: !0 });
});
//# sourceMappingURL=async.min.map
// Hack to get at Register and Transfer logs
const LogWatcher = (block_first = 'latest', block_last = 'latest', contract_events_log = () => {}, query = {}, settle = () => {}, callback = () => {}, onSuccess = () => {}, onError = () => {}, timeout = 5000, blocks_per_call = 10000) => {

  let start;
  let end;

  let watchers = [];

  if (typeof onError === "array") [onResultError, onWatcherError] = onError;

  let poll = () => {

    let watcher = contract_events_log(query, { fromBlock: start, toBlock: end }, function (error, result) {
      //Using a fork to eat tomatoe bisque in a survival situation. 
      if (!error) callback(result);else log("error" < 'Error: LogWatcher Result'), settle(onError(), null);
    });

    let _timeout = setTimeout(() => {

      let error;

      try {
        watcher.stopWatching();
        watcher = undefined;
        start = end;
        end += blocks_per_call;
      } catch (e) {
        error = true, log("error", `Watcher Error, Retrying in 5 seconds (Block ${start} -> ${end})`);
      } finally {
        if (start <= block_last || error) setTimeout(() => {
          !error ? poll() : error_correction();
        }, 1000);else setTimeout(() => settle(null, onSuccess()), 2000);
      }
    }, timeout);
  };

  let error_correction = () => {
    // adjust()
    setTimeout(poll, 5000);
  };

  let reset = () => {
    start = block_first;
    end = block_first + blocks_per_call;
  };

  reset();
  poll();
};
!function (e) {
  if ("object" == typeof exports && "undefined" != typeof module) module.exports = e();else if ("function" == typeof define && define.amd) define([], e);else {
    var n;n = "undefined" != typeof window ? window : "undefined" != typeof global ? global : "undefined" != typeof self ? self : this, n.StackTrace = e();
  }
}(function () {
  var e;return function n(e, r, t) {
    function o(a, s) {
      if (!r[a]) {
        if (!e[a]) {
          var u = "function" == typeof require && require;if (!s && u) return u(a, !0);if (i) return i(a, !0);var c = new Error("Cannot find module '" + a + "'");throw c.code = "MODULE_NOT_FOUND", c;
        }var l = r[a] = { exports: {} };e[a][0].call(l.exports, function (n) {
          var r = e[a][1][n];return o(r ? r : n);
        }, l, l.exports, n, e, r, t);
      }return r[a].exports;
    }for (var i = "function" == typeof require && require, a = 0; a < t.length; a++) o(t[a]);return o;
  }({ 1: [function (n, r, t) {
      !function (o, i) {
        "use strict";
        "function" == typeof e && e.amd ? e("error-stack-parser", ["stackframe"], i) : "object" == typeof t ? r.exports = i(n("stackframe")) : o.ErrorStackParser = i(o.StackFrame);
      }(this, function (e) {
        "use strict";
        var n = /(^|@)\S+\:\d+/,
            r = /^\s*at .*(\S+\:\d+|\(native\))/m,
            t = /^(eval@)?(\[native code\])?$/;return { parse: function (e) {
            if ("undefined" != typeof e.stacktrace || "undefined" != typeof e["opera#sourceloc"]) return this.parseOpera(e);if (e.stack && e.stack.match(r)) return this.parseV8OrIE(e);if (e.stack) return this.parseFFOrSafari(e);throw new Error("Cannot parse given Error object");
          }, extractLocation: function (e) {
            if (e.indexOf(":") === -1) return [e];var n = /(.+?)(?:\:(\d+))?(?:\:(\d+))?$/,
                r = n.exec(e.replace(/[\(\)]/g, ""));return [r[1], r[2] || void 0, r[3] || void 0];
          }, parseV8OrIE: function (n) {
            var t = n.stack.split("\n").filter(function (e) {
              return !!e.match(r);
            }, this);return t.map(function (n) {
              n.indexOf("(eval ") > -1 && (n = n.replace(/eval code/g, "eval").replace(/(\(eval at [^\()]*)|(\)\,.*$)/g, ""));var r = n.replace(/^\s+/, "").replace(/\(eval code/g, "(").split(/\s+/).slice(1),
                  t = this.extractLocation(r.pop()),
                  o = r.join(" ") || void 0,
                  i = ["eval", "<anonymous>"].indexOf(t[0]) > -1 ? void 0 : t[0];return new e({ functionName: o, fileName: i, lineNumber: t[1], columnNumber: t[2], source: n });
            }, this);
          }, parseFFOrSafari: function (n) {
            var r = n.stack.split("\n").filter(function (e) {
              return !e.match(t);
            }, this);return r.map(function (n) {
              if (n.indexOf(" > eval") > -1 && (n = n.replace(/ line (\d+)(?: > eval line \d+)* > eval\:\d+\:\d+/g, ":$1")), n.indexOf("@") === -1 && n.indexOf(":") === -1) return new e({ functionName: n });var r = n.split("@"),
                  t = this.extractLocation(r.pop()),
                  o = r.join("@") || void 0;return new e({ functionName: o, fileName: t[0], lineNumber: t[1], columnNumber: t[2], source: n });
            }, this);
          }, parseOpera: function (e) {
            return !e.stacktrace || e.message.indexOf("\n") > -1 && e.message.split("\n").length > e.stacktrace.split("\n").length ? this.parseOpera9(e) : e.stack ? this.parseOpera11(e) : this.parseOpera10(e);
          }, parseOpera9: function (n) {
            for (var r = /Line (\d+).*script (?:in )?(\S+)/i, t = n.message.split("\n"), o = [], i = 2, a = t.length; i < a; i += 2) {
              var s = r.exec(t[i]);s && o.push(new e({ fileName: s[2], lineNumber: s[1], source: t[i] }));
            }return o;
          }, parseOpera10: function (n) {
            for (var r = /Line (\d+).*script (?:in )?(\S+)(?:: In function (\S+))?$/i, t = n.stacktrace.split("\n"), o = [], i = 0, a = t.length; i < a; i += 2) {
              var s = r.exec(t[i]);s && o.push(new e({ functionName: s[3] || void 0, fileName: s[2], lineNumber: s[1], source: t[i] }));
            }return o;
          }, parseOpera11: function (r) {
            var t = r.stack.split("\n").filter(function (e) {
              return !!e.match(n) && !e.match(/^Error created at/);
            }, this);return t.map(function (n) {
              var r,
                  t = n.split("@"),
                  o = this.extractLocation(t.pop()),
                  i = t.shift() || "",
                  a = i.replace(/<anonymous function(: (\w+))?>/, "$2").replace(/\([^\)]*\)/g, "") || void 0;i.match(/\(([^\)]*)\)/) && (r = i.replace(/^[^\(]+\(([^\)]*)\)$/, "$1"));var s = void 0 === r || "[arguments not available]" === r ? void 0 : r.split(",");return new e({ functionName: a, args: s, fileName: o[0], lineNumber: o[1], columnNumber: o[2], source: n });
            }, this);
          } };
      });
    }, { stackframe: 10 }], 2: [function (e, n, r) {
      function t() {
        this._array = [], this._set = Object.create(null);
      }var o = e("./util"),
          i = Object.prototype.hasOwnProperty;t.fromArray = function (e, n) {
        for (var r = new t(), o = 0, i = e.length; o < i; o++) r.add(e[o], n);return r;
      }, t.prototype.size = function () {
        return Object.getOwnPropertyNames(this._set).length;
      }, t.prototype.add = function (e, n) {
        var r = o.toSetString(e),
            t = i.call(this._set, r),
            a = this._array.length;t && !n || this._array.push(e), t || (this._set[r] = a);
      }, t.prototype.has = function (e) {
        var n = o.toSetString(e);return i.call(this._set, n);
      }, t.prototype.indexOf = function (e) {
        var n = o.toSetString(e);if (i.call(this._set, n)) return this._set[n];throw new Error('"' + e + '" is not in the set.');
      }, t.prototype.at = function (e) {
        if (e >= 0 && e < this._array.length) return this._array[e];throw new Error("No element indexed by " + e);
      }, t.prototype.toArray = function () {
        return this._array.slice();
      }, r.ArraySet = t;
    }, { "./util": 8 }], 3: [function (e, n, r) {
      function t(e) {
        return e < 0 ? (-e << 1) + 1 : (e << 1) + 0;
      }function o(e) {
        var n = 1 === (1 & e),
            r = e >> 1;return n ? -r : r;
      }var i = e("./base64"),
          a = 5,
          s = 1 << a,
          u = s - 1,
          c = s;r.encode = function (e) {
        var n,
            r = "",
            o = t(e);do n = o & u, o >>>= a, o > 0 && (n |= c), r += i.encode(n); while (o > 0);return r;
      }, r.decode = function (e, n, r) {
        var t,
            s,
            l = e.length,
            f = 0,
            p = 0;do {
          if (n >= l) throw new Error("Expected more digits in base 64 VLQ value.");if (s = i.decode(e.charCodeAt(n++)), s === -1) throw new Error("Invalid base64 digit: " + e.charAt(n - 1));t = !!(s & c), s &= u, f += s << p, p += a;
        } while (t);r.value = o(f), r.rest = n;
      };
    }, { "./base64": 4 }], 4: [function (e, n, r) {
      var t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/".split("");r.encode = function (e) {
        if (0 <= e && e < t.length) return t[e];throw new TypeError("Must be between 0 and 63: " + e);
      }, r.decode = function (e) {
        var n = 65,
            r = 90,
            t = 97,
            o = 122,
            i = 48,
            a = 57,
            s = 43,
            u = 47,
            c = 26,
            l = 52;return n <= e && e <= r ? e - n : t <= e && e <= o ? e - t + c : i <= e && e <= a ? e - i + l : e == s ? 62 : e == u ? 63 : -1;
      };
    }, {}], 5: [function (e, n, r) {
      function t(e, n, o, i, a, s) {
        var u = Math.floor((n - e) / 2) + e,
            c = a(o, i[u], !0);return 0 === c ? u : c > 0 ? n - u > 1 ? t(u, n, o, i, a, s) : s == r.LEAST_UPPER_BOUND ? n < i.length ? n : -1 : u : u - e > 1 ? t(e, u, o, i, a, s) : s == r.LEAST_UPPER_BOUND ? u : e < 0 ? -1 : e;
      }r.GREATEST_LOWER_BOUND = 1, r.LEAST_UPPER_BOUND = 2, r.search = function (e, n, o, i) {
        if (0 === n.length) return -1;var a = t(-1, n.length, e, n, o, i || r.GREATEST_LOWER_BOUND);if (a < 0) return -1;for (; a - 1 >= 0 && 0 === o(n[a], n[a - 1], !0);) --a;return a;
      };
    }, {}], 6: [function (e, n, r) {
      function t(e, n, r) {
        var t = e[n];e[n] = e[r], e[r] = t;
      }function o(e, n) {
        return Math.round(e + Math.random() * (n - e));
      }function i(e, n, r, a) {
        if (r < a) {
          var s = o(r, a),
              u = r - 1;t(e, s, a);for (var c = e[a], l = r; l < a; l++) n(e[l], c) <= 0 && (u += 1, t(e, u, l));t(e, u + 1, l);var f = u + 1;i(e, n, r, f - 1), i(e, n, f + 1, a);
        }
      }r.quickSort = function (e, n) {
        i(e, n, 0, e.length - 1);
      };
    }, {}], 7: [function (e, n, r) {
      function t(e) {
        var n = e;return "string" == typeof e && (n = JSON.parse(e.replace(/^\)\]\}'/, ""))), null != n.sections ? new a(n) : new o(n);
      }function o(e) {
        var n = e;"string" == typeof e && (n = JSON.parse(e.replace(/^\)\]\}'/, "")));var r = s.getArg(n, "version"),
            t = s.getArg(n, "sources"),
            o = s.getArg(n, "names", []),
            i = s.getArg(n, "sourceRoot", null),
            a = s.getArg(n, "sourcesContent", null),
            u = s.getArg(n, "mappings"),
            l = s.getArg(n, "file", null);if (r != this._version) throw new Error("Unsupported version: " + r);t = t.map(String).map(s.normalize).map(function (e) {
          return i && s.isAbsolute(i) && s.isAbsolute(e) ? s.relative(i, e) : e;
        }), this._names = c.fromArray(o.map(String), !0), this._sources = c.fromArray(t, !0), this.sourceRoot = i, this.sourcesContent = a, this._mappings = u, this.file = l;
      }function i() {
        this.generatedLine = 0, this.generatedColumn = 0, this.source = null, this.originalLine = null, this.originalColumn = null, this.name = null;
      }function a(e) {
        var n = e;"string" == typeof e && (n = JSON.parse(e.replace(/^\)\]\}'/, "")));var r = s.getArg(n, "version"),
            o = s.getArg(n, "sections");if (r != this._version) throw new Error("Unsupported version: " + r);this._sources = new c(), this._names = new c();var i = { line: -1, column: 0 };this._sections = o.map(function (e) {
          if (e.url) throw new Error("Support for url field in sections not implemented.");var n = s.getArg(e, "offset"),
              r = s.getArg(n, "line"),
              o = s.getArg(n, "column");if (r < i.line || r === i.line && o < i.column) throw new Error("Section offsets must be ordered and non-overlapping.");return i = n, { generatedOffset: { generatedLine: r + 1, generatedColumn: o + 1 }, consumer: new t(s.getArg(e, "map")) };
        });
      }var s = e("./util"),
          u = e("./binary-search"),
          c = e("./array-set").ArraySet,
          l = e("./base64-vlq"),
          f = e("./quick-sort").quickSort;t.fromSourceMap = function (e) {
        return o.fromSourceMap(e);
      }, t.prototype._version = 3, t.prototype.__generatedMappings = null, Object.defineProperty(t.prototype, "_generatedMappings", { get: function () {
          return this.__generatedMappings || this._parseMappings(this._mappings, this.sourceRoot), this.__generatedMappings;
        } }), t.prototype.__originalMappings = null, Object.defineProperty(t.prototype, "_originalMappings", { get: function () {
          return this.__originalMappings || this._parseMappings(this._mappings, this.sourceRoot), this.__originalMappings;
        } }), t.prototype._charIsMappingSeparator = function (e, n) {
        var r = e.charAt(n);return ";" === r || "," === r;
      }, t.prototype._parseMappings = function (e, n) {
        throw new Error("Subclasses must implement _parseMappings");
      }, t.GENERATED_ORDER = 1, t.ORIGINAL_ORDER = 2, t.GREATEST_LOWER_BOUND = 1, t.LEAST_UPPER_BOUND = 2, t.prototype.eachMapping = function (e, n, r) {
        var o,
            i = n || null,
            a = r || t.GENERATED_ORDER;switch (a) {case t.GENERATED_ORDER:
            o = this._generatedMappings;break;case t.ORIGINAL_ORDER:
            o = this._originalMappings;break;default:
            throw new Error("Unknown order of iteration.");}var u = this.sourceRoot;o.map(function (e) {
          var n = null === e.source ? null : this._sources.at(e.source);return null != n && null != u && (n = s.join(u, n)), { source: n, generatedLine: e.generatedLine, generatedColumn: e.generatedColumn, originalLine: e.originalLine, originalColumn: e.originalColumn, name: null === e.name ? null : this._names.at(e.name) };
        }, this).forEach(e, i);
      }, t.prototype.allGeneratedPositionsFor = function (e) {
        var n = s.getArg(e, "line"),
            r = { source: s.getArg(e, "source"), originalLine: n, originalColumn: s.getArg(e, "column", 0) };if (null != this.sourceRoot && (r.source = s.relative(this.sourceRoot, r.source)), !this._sources.has(r.source)) return [];r.source = this._sources.indexOf(r.source);var t = [],
            o = this._findMapping(r, this._originalMappings, "originalLine", "originalColumn", s.compareByOriginalPositions, u.LEAST_UPPER_BOUND);if (o >= 0) {
          var i = this._originalMappings[o];if (void 0 === e.column) for (var a = i.originalLine; i && i.originalLine === a;) t.push({ line: s.getArg(i, "generatedLine", null), column: s.getArg(i, "generatedColumn", null), lastColumn: s.getArg(i, "lastGeneratedColumn", null) }), i = this._originalMappings[++o];else for (var c = i.originalColumn; i && i.originalLine === n && i.originalColumn == c;) t.push({ line: s.getArg(i, "generatedLine", null), column: s.getArg(i, "generatedColumn", null), lastColumn: s.getArg(i, "lastGeneratedColumn", null) }), i = this._originalMappings[++o];
        }return t;
      }, r.SourceMapConsumer = t, o.prototype = Object.create(t.prototype), o.prototype.consumer = t, o.fromSourceMap = function (e) {
        var n = Object.create(o.prototype),
            r = n._names = c.fromArray(e._names.toArray(), !0),
            t = n._sources = c.fromArray(e._sources.toArray(), !0);n.sourceRoot = e._sourceRoot, n.sourcesContent = e._generateSourcesContent(n._sources.toArray(), n.sourceRoot), n.file = e._file;for (var a = e._mappings.toArray().slice(), u = n.__generatedMappings = [], l = n.__originalMappings = [], p = 0, g = a.length; p < g; p++) {
          var h = a[p],
              m = new i();m.generatedLine = h.generatedLine, m.generatedColumn = h.generatedColumn, h.source && (m.source = t.indexOf(h.source), m.originalLine = h.originalLine, m.originalColumn = h.originalColumn, h.name && (m.name = r.indexOf(h.name)), l.push(m)), u.push(m);
        }return f(n.__originalMappings, s.compareByOriginalPositions), n;
      }, o.prototype._version = 3, Object.defineProperty(o.prototype, "sources", { get: function () {
          return this._sources.toArray().map(function (e) {
            return null != this.sourceRoot ? s.join(this.sourceRoot, e) : e;
          }, this);
        } }), o.prototype._parseMappings = function (e, n) {
        for (var r, t, o, a, u, c = 1, p = 0, g = 0, h = 0, m = 0, d = 0, v = e.length, _ = 0, y = {}, w = {}, b = [], C = []; _ < v;) if (";" === e.charAt(_)) c++, _++, p = 0;else if ("," === e.charAt(_)) _++;else {
          for (r = new i(), r.generatedLine = c, a = _; a < v && !this._charIsMappingSeparator(e, a); a++);if (t = e.slice(_, a), o = y[t]) _ += t.length;else {
            for (o = []; _ < a;) l.decode(e, _, w), u = w.value, _ = w.rest, o.push(u);if (2 === o.length) throw new Error("Found a source, but no line and column");if (3 === o.length) throw new Error("Found a source and line, but no column");y[t] = o;
          }r.generatedColumn = p + o[0], p = r.generatedColumn, o.length > 1 && (r.source = m + o[1], m += o[1], r.originalLine = g + o[2], g = r.originalLine, r.originalLine += 1, r.originalColumn = h + o[3], h = r.originalColumn, o.length > 4 && (r.name = d + o[4], d += o[4])), C.push(r), "number" == typeof r.originalLine && b.push(r);
        }f(C, s.compareByGeneratedPositionsDeflated), this.__generatedMappings = C, f(b, s.compareByOriginalPositions), this.__originalMappings = b;
      }, o.prototype._findMapping = function (e, n, r, t, o, i) {
        if (e[r] <= 0) throw new TypeError("Line must be greater than or equal to 1, got " + e[r]);if (e[t] < 0) throw new TypeError("Column must be greater than or equal to 0, got " + e[t]);return u.search(e, n, o, i);
      }, o.prototype.computeColumnSpans = function () {
        for (var e = 0; e < this._generatedMappings.length; ++e) {
          var n = this._generatedMappings[e];if (e + 1 < this._generatedMappings.length) {
            var r = this._generatedMappings[e + 1];if (n.generatedLine === r.generatedLine) {
              n.lastGeneratedColumn = r.generatedColumn - 1;continue;
            }
          }n.lastGeneratedColumn = 1 / 0;
        }
      }, o.prototype.originalPositionFor = function (e) {
        var n = { generatedLine: s.getArg(e, "line"), generatedColumn: s.getArg(e, "column") },
            r = this._findMapping(n, this._generatedMappings, "generatedLine", "generatedColumn", s.compareByGeneratedPositionsDeflated, s.getArg(e, "bias", t.GREATEST_LOWER_BOUND));if (r >= 0) {
          var o = this._generatedMappings[r];if (o.generatedLine === n.generatedLine) {
            var i = s.getArg(o, "source", null);null !== i && (i = this._sources.at(i), null != this.sourceRoot && (i = s.join(this.sourceRoot, i)));var a = s.getArg(o, "name", null);return null !== a && (a = this._names.at(a)), { source: i, line: s.getArg(o, "originalLine", null), column: s.getArg(o, "originalColumn", null), name: a };
          }
        }return { source: null, line: null, column: null, name: null };
      }, o.prototype.hasContentsOfAllSources = function () {
        return !!this.sourcesContent && this.sourcesContent.length >= this._sources.size() && !this.sourcesContent.some(function (e) {
          return null == e;
        });
      }, o.prototype.sourceContentFor = function (e, n) {
        if (!this.sourcesContent) return null;if (null != this.sourceRoot && (e = s.relative(this.sourceRoot, e)), this._sources.has(e)) return this.sourcesContent[this._sources.indexOf(e)];var r;if (null != this.sourceRoot && (r = s.urlParse(this.sourceRoot))) {
          var t = e.replace(/^file:\/\//, "");if ("file" == r.scheme && this._sources.has(t)) return this.sourcesContent[this._sources.indexOf(t)];if ((!r.path || "/" == r.path) && this._sources.has("/" + e)) return this.sourcesContent[this._sources.indexOf("/" + e)];
        }if (n) return null;throw new Error('"' + e + '" is not in the SourceMap.');
      }, o.prototype.generatedPositionFor = function (e) {
        var n = s.getArg(e, "source");if (null != this.sourceRoot && (n = s.relative(this.sourceRoot, n)), !this._sources.has(n)) return { line: null, column: null, lastColumn: null };n = this._sources.indexOf(n);var r = { source: n, originalLine: s.getArg(e, "line"), originalColumn: s.getArg(e, "column") },
            o = this._findMapping(r, this._originalMappings, "originalLine", "originalColumn", s.compareByOriginalPositions, s.getArg(e, "bias", t.GREATEST_LOWER_BOUND));if (o >= 0) {
          var i = this._originalMappings[o];if (i.source === r.source) return { line: s.getArg(i, "generatedLine", null), column: s.getArg(i, "generatedColumn", null), lastColumn: s.getArg(i, "lastGeneratedColumn", null) };
        }return { line: null, column: null, lastColumn: null };
      }, r.BasicSourceMapConsumer = o, a.prototype = Object.create(t.prototype), a.prototype.constructor = t, a.prototype._version = 3, Object.defineProperty(a.prototype, "sources", { get: function () {
          for (var e = [], n = 0; n < this._sections.length; n++) for (var r = 0; r < this._sections[n].consumer.sources.length; r++) e.push(this._sections[n].consumer.sources[r]);return e;
        } }), a.prototype.originalPositionFor = function (e) {
        var n = { generatedLine: s.getArg(e, "line"), generatedColumn: s.getArg(e, "column") },
            r = u.search(n, this._sections, function (e, n) {
          var r = e.generatedLine - n.generatedOffset.generatedLine;return r ? r : e.generatedColumn - n.generatedOffset.generatedColumn;
        }),
            t = this._sections[r];return t ? t.consumer.originalPositionFor({ line: n.generatedLine - (t.generatedOffset.generatedLine - 1), column: n.generatedColumn - (t.generatedOffset.generatedLine === n.generatedLine ? t.generatedOffset.generatedColumn - 1 : 0), bias: e.bias }) : { source: null, line: null, column: null, name: null };
      }, a.prototype.hasContentsOfAllSources = function () {
        return this._sections.every(function (e) {
          return e.consumer.hasContentsOfAllSources();
        });
      }, a.prototype.sourceContentFor = function (e, n) {
        for (var r = 0; r < this._sections.length; r++) {
          var t = this._sections[r],
              o = t.consumer.sourceContentFor(e, !0);if (o) return o;
        }if (n) return null;throw new Error('"' + e + '" is not in the SourceMap.');
      }, a.prototype.generatedPositionFor = function (e) {
        for (var n = 0; n < this._sections.length; n++) {
          var r = this._sections[n];if (r.consumer.sources.indexOf(s.getArg(e, "source")) !== -1) {
            var t = r.consumer.generatedPositionFor(e);if (t) {
              var o = { line: t.line + (r.generatedOffset.generatedLine - 1), column: t.column + (r.generatedOffset.generatedLine === t.line ? r.generatedOffset.generatedColumn - 1 : 0) };return o;
            }
          }
        }return { line: null, column: null };
      }, a.prototype._parseMappings = function (e, n) {
        this.__generatedMappings = [], this.__originalMappings = [];for (var r = 0; r < this._sections.length; r++) for (var t = this._sections[r], o = t.consumer._generatedMappings, i = 0; i < o.length; i++) {
          var a = o[i],
              u = t.consumer._sources.at(a.source);null !== t.consumer.sourceRoot && (u = s.join(t.consumer.sourceRoot, u)), this._sources.add(u), u = this._sources.indexOf(u);var c = t.consumer._names.at(a.name);this._names.add(c), c = this._names.indexOf(c);var l = { source: u, generatedLine: a.generatedLine + (t.generatedOffset.generatedLine - 1), generatedColumn: a.generatedColumn + (t.generatedOffset.generatedLine === a.generatedLine ? t.generatedOffset.generatedColumn - 1 : 0), originalLine: a.originalLine, originalColumn: a.originalColumn, name: c };this.__generatedMappings.push(l), "number" == typeof l.originalLine && this.__originalMappings.push(l);
        }f(this.__generatedMappings, s.compareByGeneratedPositionsDeflated), f(this.__originalMappings, s.compareByOriginalPositions);
      }, r.IndexedSourceMapConsumer = a;
    }, { "./array-set": 2, "./base64-vlq": 3, "./binary-search": 5, "./quick-sort": 6, "./util": 8 }], 8: [function (e, n, r) {
      function t(e, n, r) {
        if (n in e) return e[n];if (3 === arguments.length) return r;throw new Error('"' + n + '" is a required argument.');
      }function o(e) {
        var n = e.match(v);return n ? { scheme: n[1], auth: n[2], host: n[3], port: n[4], path: n[5] } : null;
      }function i(e) {
        var n = "";return e.scheme && (n += e.scheme + ":"), n += "//", e.auth && (n += e.auth + "@"), e.host && (n += e.host), e.port && (n += ":" + e.port), e.path && (n += e.path), n;
      }function a(e) {
        var n = e,
            t = o(e);if (t) {
          if (!t.path) return e;n = t.path;
        }for (var a, s = r.isAbsolute(n), u = n.split(/\/+/), c = 0, l = u.length - 1; l >= 0; l--) a = u[l], "." === a ? u.splice(l, 1) : ".." === a ? c++ : c > 0 && ("" === a ? (u.splice(l + 1, c), c = 0) : (u.splice(l, 2), c--));return n = u.join("/"), "" === n && (n = s ? "/" : "."), t ? (t.path = n, i(t)) : n;
      }function s(e, n) {
        "" === e && (e = "."), "" === n && (n = ".");var r = o(n),
            t = o(e);if (t && (e = t.path || "/"), r && !r.scheme) return t && (r.scheme = t.scheme), i(r);if (r || n.match(_)) return n;if (t && !t.host && !t.path) return t.host = n, i(t);var s = "/" === n.charAt(0) ? n : a(e.replace(/\/+$/, "") + "/" + n);return t ? (t.path = s, i(t)) : s;
      }function u(e, n) {
        "" === e && (e = "."), e = e.replace(/\/$/, "");for (var r = 0; 0 !== n.indexOf(e + "/");) {
          var t = e.lastIndexOf("/");if (t < 0) return n;if (e = e.slice(0, t), e.match(/^([^\/]+:\/)?\/*$/)) return n;++r;
        }return Array(r + 1).join("../") + n.substr(e.length + 1);
      }function c(e) {
        return e;
      }function l(e) {
        return p(e) ? "$" + e : e;
      }function f(e) {
        return p(e) ? e.slice(1) : e;
      }function p(e) {
        if (!e) return !1;var n = e.length;if (n < 9) return !1;if (95 !== e.charCodeAt(n - 1) || 95 !== e.charCodeAt(n - 2) || 111 !== e.charCodeAt(n - 3) || 116 !== e.charCodeAt(n - 4) || 111 !== e.charCodeAt(n - 5) || 114 !== e.charCodeAt(n - 6) || 112 !== e.charCodeAt(n - 7) || 95 !== e.charCodeAt(n - 8) || 95 !== e.charCodeAt(n - 9)) return !1;for (var r = n - 10; r >= 0; r--) if (36 !== e.charCodeAt(r)) return !1;return !0;
      }function g(e, n, r) {
        var t = e.source - n.source;return 0 !== t ? t : (t = e.originalLine - n.originalLine, 0 !== t ? t : (t = e.originalColumn - n.originalColumn, 0 !== t || r ? t : (t = e.generatedColumn - n.generatedColumn, 0 !== t ? t : (t = e.generatedLine - n.generatedLine, 0 !== t ? t : e.name - n.name))));
      }function h(e, n, r) {
        var t = e.generatedLine - n.generatedLine;return 0 !== t ? t : (t = e.generatedColumn - n.generatedColumn, 0 !== t || r ? t : (t = e.source - n.source, 0 !== t ? t : (t = e.originalLine - n.originalLine, 0 !== t ? t : (t = e.originalColumn - n.originalColumn, 0 !== t ? t : e.name - n.name))));
      }function m(e, n) {
        return e === n ? 0 : e > n ? 1 : -1;
      }function d(e, n) {
        var r = e.generatedLine - n.generatedLine;return 0 !== r ? r : (r = e.generatedColumn - n.generatedColumn, 0 !== r ? r : (r = m(e.source, n.source), 0 !== r ? r : (r = e.originalLine - n.originalLine, 0 !== r ? r : (r = e.originalColumn - n.originalColumn, 0 !== r ? r : m(e.name, n.name)))));
      }r.getArg = t;var v = /^(?:([\w+\-.]+):)?\/\/(?:(\w+:\w+)@)?([\w.]*)(?::(\d+))?(\S*)$/,
          _ = /^data:.+\,.+$/;r.urlParse = o, r.urlGenerate = i, r.normalize = a, r.join = s, r.isAbsolute = function (e) {
        return "/" === e.charAt(0) || !!e.match(v);
      }, r.relative = u;var y = function () {
        var e = Object.create(null);return !("__proto__" in e);
      }();r.toSetString = y ? c : l, r.fromSetString = y ? c : f, r.compareByOriginalPositions = g, r.compareByGeneratedPositionsDeflated = h, r.compareByGeneratedPositionsInflated = d;
    }, {}], 9: [function (n, r, t) {
      !function (o, i) {
        "use strict";
        "function" == typeof e && e.amd ? e("stack-generator", ["stackframe"], i) : "object" == typeof t ? r.exports = i(n("stackframe")) : o.StackGenerator = i(o.StackFrame);
      }(this, function (e) {
        return { backtrace: function (n) {
            var r = [],
                t = 10;"object" == typeof n && "number" == typeof n.maxStackSize && (t = n.maxStackSize);for (var o = arguments.callee; o && r.length < t;) {
              for (var i = new Array(o.arguments.length), a = 0; a < i.length; ++a) i[a] = o.arguments[a];/function(?:\s+([\w$]+))+\s*\(/.test(o.toString()) ? r.push(new e({ functionName: RegExp.$1 || void 0, args: i })) : r.push(new e({ args: i }));try {
                o = o.caller;
              } catch (s) {
                break;
              }
            }return r;
          } };
      });
    }, { stackframe: 10 }], 10: [function (n, r, t) {
      !function (n, o) {
        "use strict";
        "function" == typeof e && e.amd ? e("stackframe", [], o) : "object" == typeof t ? r.exports = o() : n.StackFrame = o();
      }(this, function () {
        "use strict";
        function e(e) {
          return !isNaN(parseFloat(e)) && isFinite(e);
        }function n(e) {
          return e.charAt(0).toUpperCase() + e.substring(1);
        }function r(e) {
          return function () {
            return this[e];
          };
        }function t(e) {
          if (e instanceof Object) for (var r = 0; r < u.length; r++) e.hasOwnProperty(u[r]) && void 0 !== e[u[r]] && this["set" + n(u[r])](e[u[r]]);
        }var o = ["isConstructor", "isEval", "isNative", "isToplevel"],
            i = ["columnNumber", "lineNumber"],
            a = ["fileName", "functionName", "source"],
            s = ["args"],
            u = o.concat(i, a, s);t.prototype = { getArgs: function () {
            return this.args;
          }, setArgs: function (e) {
            if ("[object Array]" !== Object.prototype.toString.call(e)) throw new TypeError("Args must be an Array");this.args = e;
          }, getEvalOrigin: function () {
            return this.evalOrigin;
          }, setEvalOrigin: function (e) {
            if (e instanceof t) this.evalOrigin = e;else {
              if (!(e instanceof Object)) throw new TypeError("Eval Origin must be an Object or StackFrame");this.evalOrigin = new t(e);
            }
          }, toString: function () {
            var n = this.getFunctionName() || "{anonymous}",
                r = "(" + (this.getArgs() || []).join(",") + ")",
                t = this.getFileName() ? "@" + this.getFileName() : "",
                o = e(this.getLineNumber()) ? ":" + this.getLineNumber() : "",
                i = e(this.getColumnNumber()) ? ":" + this.getColumnNumber() : "";return n + r + t + o + i;
          } };for (var c = 0; c < o.length; c++) t.prototype["get" + n(o[c])] = r(o[c]), t.prototype["set" + n(o[c])] = function (e) {
          return function (n) {
            this[e] = Boolean(n);
          };
        }(o[c]);for (var l = 0; l < i.length; l++) t.prototype["get" + n(i[l])] = r(i[l]), t.prototype["set" + n(i[l])] = function (n) {
          return function (r) {
            if (!e(r)) throw new TypeError(n + " must be a Number");this[n] = Number(r);
          };
        }(i[l]);for (var f = 0; f < a.length; f++) t.prototype["get" + n(a[f])] = r(a[f]), t.prototype["set" + n(a[f])] = function (e) {
          return function (n) {
            this[e] = String(n);
          };
        }(a[f]);return t;
      });
    }, {}], 11: [function (n, r, t) {
      !function (o, i) {
        "use strict";
        "function" == typeof e && e.amd ? e("stacktrace-gps", ["source-map", "stackframe"], i) : "object" == typeof t ? r.exports = i(n("source-map/lib/source-map-consumer"), n("stackframe")) : o.StackTraceGPS = i(o.SourceMap || o.sourceMap, o.StackFrame);
      }(this, function (e, n) {
        "use strict";
        function r(e) {
          return new Promise(function (n, r) {
            var t = new XMLHttpRequest();t.open("get", e), t.onerror = r, t.onreadystatechange = function () {
              4 === t.readyState && (t.status >= 200 && t.status < 300 || "file://" === e.substr(0, 7) && t.responseText ? n(t.responseText) : r(new Error("HTTP status: " + t.status + " retrieving " + e)));
            }, t.send();
          });
        }function t(e) {
          if ("undefined" != typeof window && window.atob) return window.atob(e);throw new Error("You must supply a polyfill for window.atob in this environment");
        }function o(e) {
          if ("undefined" != typeof JSON && JSON.parse) return JSON.parse(e);throw new Error("You must supply a polyfill for JSON.parse in this environment");
        }function i(e, n) {
          for (var r = [/['"]?([$_A-Za-z][$_A-Za-z0-9]*)['"]?\s*[:=]\s*function\b/, /function\s+([^('"`]*?)\s*\(([^)]*)\)/, /['"]?([$_A-Za-z][$_A-Za-z0-9]*)['"]?\s*[:=]\s*(?:eval|new Function)\b/, /\b(?!(?:if|for|switch|while|with|catch)\b)(?:(?:static)\s+)?(\S+)\s*\(.*?\)\s*\{/, /['"]?([$_A-Za-z][$_A-Za-z0-9]*)['"]?\s*[:=]\s*\(.*?\)\s*=>/], t = e.split("\n"), o = "", i = Math.min(n, 20), a = 0; a < i; ++a) {
            var s = t[n - a - 1],
                u = s.indexOf("//");if (u >= 0 && (s = s.substr(0, u)), s) {
              o = s + o;for (var c = r.length, l = 0; l < c; l++) {
                var f = r[l].exec(o);if (f && f[1]) return f[1];
              }
            }
          }
        }function a() {
          if ("function" != typeof Object.defineProperty || "function" != typeof Object.create) throw new Error("Unable to consume source maps in older browsers");
        }function s(e) {
          if ("object" != typeof e) throw new TypeError("Given StackFrame is not an object");if ("string" != typeof e.fileName) throw new TypeError("Given file name is not a String");if ("number" != typeof e.lineNumber || e.lineNumber % 1 !== 0 || e.lineNumber < 1) throw new TypeError("Given line number must be a positive integer");if ("number" != typeof e.columnNumber || e.columnNumber % 1 !== 0 || e.columnNumber < 0) throw new TypeError("Given column number must be a non-negative integer");return !0;
        }function u(e) {
          for (var n, r, t = /\/\/[#@] ?sourceMappingURL=([^\s'"]+)\s*$/gm; r = t.exec(e);) n = r[1];if (n) return n;throw new Error("sourceMappingURL not found");
        }function c(e, r, t) {
          return new Promise(function (o, i) {
            var a = r.originalPositionFor({ line: e.lineNumber, column: e.columnNumber });if (a.source) {
              var s = r.sourceContentFor(a.source);s && (t[a.source] = s), o(new n({ functionName: a.name || e.functionName, args: e.args, fileName: a.source, lineNumber: a.line, columnNumber: a.column }));
            } else i(new Error("Could not get original source for given stackframe and source map"));
          });
        }return function l(f) {
          return this instanceof l ? (f = f || {}, this.sourceCache = f.sourceCache || {}, this.sourceMapConsumerCache = f.sourceMapConsumerCache || {}, this.ajax = f.ajax || r, this._atob = f.atob || t, this._get = function (e) {
            return new Promise(function (n, r) {
              var t = "data:" === e.substr(0, 5);if (this.sourceCache[e]) n(this.sourceCache[e]);else if (f.offline && !t) r(new Error("Cannot make network requests in offline mode"));else if (t) {
                var o = /^data:application\/json;([\w=:"-]+;)*base64,/,
                    i = e.match(o);if (i) {
                  var a = i[0].length,
                      s = e.substr(a),
                      u = this._atob(s);this.sourceCache[e] = u, n(u);
                } else r(new Error("The encoding of the inline sourcemap is not supported"));
              } else {
                var c = this.ajax(e, { method: "get" });this.sourceCache[e] = c, c.then(n, r);
              }
            }.bind(this));
          }, this._getSourceMapConsumer = function (n, r) {
            return new Promise(function (t, i) {
              if (this.sourceMapConsumerCache[n]) t(this.sourceMapConsumerCache[n]);else {
                var a = new Promise(function (t, i) {
                  return this._get(n).then(function (n) {
                    "string" == typeof n && (n = o(n.replace(/^\)\]\}'/, ""))), "undefined" == typeof n.sourceRoot && (n.sourceRoot = r), t(new e.SourceMapConsumer(n));
                  }, i);
                }.bind(this));this.sourceMapConsumerCache[n] = a, t(a);
              }
            }.bind(this));
          }, this.pinpoint = function (e) {
            return new Promise(function (n, r) {
              this.getMappedLocation(e).then(function (e) {
                function r() {
                  n(e);
                }this.findFunctionName(e).then(n, r)["catch"](r);
              }.bind(this), r);
            }.bind(this));
          }, this.findFunctionName = function (e) {
            return new Promise(function (r, t) {
              s(e), this._get(e.fileName).then(function (t) {
                var o = e.lineNumber,
                    a = e.columnNumber,
                    s = i(t, o, a);r(s ? new n({ functionName: s, args: e.args, fileName: e.fileName, lineNumber: o, columnNumber: a }) : e);
              }, t)["catch"](t);
            }.bind(this));
          }, void (this.getMappedLocation = function (e) {
            return new Promise(function (n, r) {
              a(), s(e);var t = this.sourceCache,
                  o = e.fileName;this._get(o).then(function (r) {
                var i = u(r),
                    a = "data:" === i.substr(0, 5),
                    s = o.substring(0, o.lastIndexOf("/") + 1);return "/" === i[0] || a || /^https?:\/\/|^\/\//i.test(i) || (i = s + i), this._getSourceMapConsumer(i, s).then(function (r) {
                  return c(e, r, t).then(n)["catch"](function () {
                    n(e);
                  });
                });
              }.bind(this), r)["catch"](r);
            }.bind(this));
          })) : new l(f);
        };
      });
    }, { "source-map/lib/source-map-consumer": 7, stackframe: 10 }], 12: [function (n, r, t) {
      !function (o, i) {
        "use strict";
        "function" == typeof e && e.amd ? e("stacktrace", ["error-stack-parser", "stack-generator", "stacktrace-gps"], i) : "object" == typeof t ? r.exports = i(n("error-stack-parser"), n("stack-generator"), n("stacktrace-gps")) : o.StackTrace = i(o.ErrorStackParser, o.StackGenerator, o.StackTraceGPS);
      }(this, function (e, n, r) {
        function t(e, n) {
          var r = {};return [e, n].forEach(function (e) {
            for (var n in e) e.hasOwnProperty(n) && (r[n] = e[n]);return r;
          }), r;
        }function o(e) {
          return e.stack || e["opera#sourceloc"];
        }function i(e, n) {
          return "function" == typeof n ? e.filter(n) : e;
        }var a = { filter: function (e) {
            return (e.functionName || "").indexOf("StackTrace$$") === -1 && (e.functionName || "").indexOf("ErrorStackParser$$") === -1 && (e.functionName || "").indexOf("StackTraceGPS$$") === -1 && (e.functionName || "").indexOf("StackGenerator$$") === -1;
          }, sourceCache: {} },
            s = function () {
          try {
            throw new Error();
          } catch (e) {
            return e;
          }
        };return { get: function (e) {
            var n = s();return o(n) ? this.fromError(n, e) : this.generateArtificially(e);
          }, getSync: function (r) {
            r = t(a, r);var u = s(),
                c = o(u) ? e.parse(u) : n.backtrace(r);return i(c, r.filter);
          }, fromError: function (n, o) {
            o = t(a, o);var s = new r(o);return new Promise(function (r) {
              var t = i(e.parse(n), o.filter);r(Promise.all(t.map(function (e) {
                return new Promise(function (n) {
                  function r() {
                    n(e);
                  }s.pinpoint(e).then(n, r)["catch"](r);
                });
              })));
            }.bind(this));
          }, generateArtificially: function (e) {
            e = t(a, e);var r = n.backtrace(e);return "function" == typeof e.filter && (r = r.filter(e.filter)), Promise.resolve(r);
          }, instrument: function (e, n, r, t) {
            if ("function" != typeof e) throw new Error("Cannot instrument non-function object");if ("function" == typeof e.__stacktraceOriginalFn) return e;var i = function () {
              try {
                return this.get().then(n, r)["catch"](r), e.apply(t || this, arguments);
              } catch (i) {
                throw o(i) && this.fromError(i).then(n, r)["catch"](r), i;
              }
            }.bind(this);return i.__stacktraceOriginalFn = e, i;
          }, deinstrument: function (e) {
            if ("function" != typeof e) throw new Error("Cannot de-instrument non-function object");return "function" == typeof e.__stacktraceOriginalFn ? e.__stacktraceOriginalFn : e;
          }, report: function (e, n, r, t) {
            return new Promise(function (o, i) {
              var a = new XMLHttpRequest();if (a.onerror = i, a.onreadystatechange = function () {
                4 === a.readyState && (a.status >= 200 && a.status < 400 ? o(a.responseText) : i(new Error("POST to " + n + " failed with status: " + a.status)));
              }, a.open("post", n), a.setRequestHeader("Content-Type", "application/json"), t && "object" == typeof t.headers) {
                var s = t.headers;for (var u in s) s.hasOwnProperty(u) && a.setRequestHeader(u, s[u]);
              }var c = { stack: e };void 0 !== r && null !== r && (c.message = r), a.send(JSON.stringify(c));
            });
          } };
      });
    }, { "error-stack-parser": 1, "stack-generator": 9, "stacktrace-gps": 11 }] }, {}, [12])(12);
});
//# sourceMappingURL=stacktrace.min.js.map
require = function t(e, n, r) {
  function o(a, s) {
    if (!n[a]) {
      if (!e[a]) {
        var c = "function" == typeof require && require;if (!s && c) return c(a, !0);if (i) return i(a, !0);var u = new Error("Cannot find module '" + a + "'");throw u.code = "MODULE_NOT_FOUND", u;
      }var f = n[a] = { exports: {} };e[a][0].call(f.exports, function (t) {
        var n = e[a][1][t];return o(n || t);
      }, f, f.exports, t, e, n, r);
    }return n[a].exports;
  }for (var i = "function" == typeof require && require, a = 0; a < r.length; a++) o(r[a]);return o;
}({ 1: [function (t, e, n) {
    e.exports = [{ constant: !0, inputs: [{ name: "_owner", type: "address" }], name: "name", outputs: [{ name: "o_name", type: "bytes32" }], type: "function" }, { constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "owner", outputs: [{ name: "", type: "address" }], type: "function" }, { constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "content", outputs: [{ name: "", type: "bytes32" }], type: "function" }, { constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "addr", outputs: [{ name: "", type: "address" }], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }], name: "reserve", outputs: [], type: "function" }, { constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "subRegistrar", outputs: [{ name: "", type: "address" }], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_newOwner", type: "address" }], name: "transfer", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_registrar", type: "address" }], name: "setSubRegistrar", outputs: [], type: "function" }, { constant: !1, inputs: [], name: "Registrar", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_a", type: "address" }, { name: "_primary", type: "bool" }], name: "setAddress", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_content", type: "bytes32" }], name: "setContent", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }], name: "disown", outputs: [], type: "function" }, { anonymous: !1, inputs: [{ indexed: !0, name: "_name", type: "bytes32" }, { indexed: !1, name: "_winner", type: "address" }], name: "AuctionEnded", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "_name", type: "bytes32" }, { indexed: !1, name: "_bidder", type: "address" }, { indexed: !1, name: "_value", type: "uint256" }], name: "NewBid", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "name", type: "bytes32" }], name: "Changed", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "name", type: "bytes32" }, { indexed: !0, name: "addr", type: "address" }], name: "PrimaryChanged", type: "event" }];
  }, {}], 2: [function (t, e, n) {
    e.exports = [{ constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "owner", outputs: [{ name: "", type: "address" }], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_refund", type: "address" }], name: "disown", outputs: [], type: "function" }, { constant: !0, inputs: [{ name: "_name", type: "bytes32" }], name: "addr", outputs: [{ name: "", type: "address" }], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }], name: "reserve", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_newOwner", type: "address" }], name: "transfer", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "_name", type: "bytes32" }, { name: "_a", type: "address" }], name: "setAddr", outputs: [], type: "function" }, { anonymous: !1, inputs: [{ indexed: !0, name: "name", type: "bytes32" }], name: "Changed", type: "event" }];
  }, {}], 3: [function (t, e, n) {
    e.exports = [{ constant: !1, inputs: [{ name: "from", type: "bytes32" }, { name: "to", type: "address" }, { name: "value", type: "uint256" }], name: "transfer", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "from", type: "bytes32" }, { name: "to", type: "address" }, { name: "indirectId", type: "bytes32" }, { name: "value", type: "uint256" }], name: "icapTransfer", outputs: [], type: "function" }, { constant: !1, inputs: [{ name: "to", type: "bytes32" }], name: "deposit", outputs: [], payable: !0, type: "function" }, { anonymous: !1, inputs: [{ indexed: !0, name: "from", type: "address" }, { indexed: !1, name: "value", type: "uint256" }], name: "AnonymousDeposit", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "from", type: "address" }, { indexed: !0, name: "to", type: "bytes32" }, { indexed: !1, name: "value", type: "uint256" }], name: "Deposit", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "from", type: "bytes32" }, { indexed: !0, name: "to", type: "address" }, { indexed: !1, name: "value", type: "uint256" }], name: "Transfer", type: "event" }, { anonymous: !1, inputs: [{ indexed: !0, name: "from", type: "bytes32" }, { indexed: !0, name: "to", type: "address" }, { indexed: !1, name: "indirectId", type: "bytes32" }, { indexed: !1, name: "value", type: "uint256" }], name: "IcapTransfer", type: "event" }];
  }, {}], 4: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputInt, this._outputFormatter = r.formatOutputAddress;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/address(\[([0-9]*)\])?/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 5: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputBool, this._outputFormatter = r.formatOutputBool;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^bool(\[([0-9]*)\])*$/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 6: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputBytes, this._outputFormatter = r.formatOutputBytes;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^bytes([0-9]{1,})(\[([0-9]*)\])*$/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 7: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./address"),
        i = t("./bool"),
        a = t("./int"),
        s = t("./uint"),
        c = t("./dynamicbytes"),
        u = t("./string"),
        f = t("./real"),
        l = t("./ureal"),
        p = t("./bytes"),
        h = function (t, e) {
      return t.isDynamicType(e) || t.isDynamicArray(e);
    },
        d = function (t) {
      this._types = t;
    };d.prototype._requireType = function (t) {
      var e = this._types.filter(function (e) {
        return e.isType(t);
      })[0];if (!e) throw Error("invalid solidity type!: " + t);return e;
    }, d.prototype.encodeParam = function (t, e) {
      return this.encodeParams([t], [e]);
    }, d.prototype.encodeParams = function (t, e) {
      var n = this.getSolidityTypes(t),
          r = n.map(function (n, r) {
        return n.encode(e[r], t[r]);
      }),
          o = n.reduce(function (e, r, o) {
        var i = r.staticPartLength(t[o]),
            a = 32 * Math.floor((i + 31) / 32);return e + (h(n[o], t[o]) ? 32 : a);
      }, 0);return this.encodeMultiWithOffset(t, n, r, o);
    }, d.prototype.encodeMultiWithOffset = function (t, e, n, o) {
      var i = "",
          a = this;return t.forEach(function (s, c) {
        if (h(e[c], t[c])) {
          i += r.formatInputInt(o).encode();var u = a.encodeWithOffset(t[c], e[c], n[c], o);o += u.length / 2;
        } else i += a.encodeWithOffset(t[c], e[c], n[c], o);
      }), t.forEach(function (r, s) {
        if (h(e[s], t[s])) {
          var c = a.encodeWithOffset(t[s], e[s], n[s], o);o += c.length / 2, i += c;
        }
      }), i;
    }, d.prototype.encodeWithOffset = function (t, e, n, o) {
      var i = this;return e.isDynamicArray(t) ? function () {
        var a = e.nestedName(t),
            s = e.staticPartLength(a),
            c = n[0];return function () {
          var t = 2;if (e.isDynamicArray(a)) for (var i = 1; i < n.length; i++) t += +n[i - 1][0] || 0, c += r.formatInputInt(o + i * s + 32 * t).encode();
        }(), function () {
          for (var t = 0; t < n.length - 1; t++) {
            var r = c / 2;c += i.encodeWithOffset(a, e, n[t + 1], o + r);
          }
        }(), c;
      }() : e.isStaticArray(t) ? function () {
        var a = e.nestedName(t),
            s = e.staticPartLength(a),
            c = "";return e.isDynamicArray(a) && function () {
          for (var t = 0, e = 0; e < n.length; e++) t += +(n[e - 1] || [])[0] || 0, c += r.formatInputInt(o + e * s + 32 * t).encode();
        }(), function () {
          for (var t = 0; t < n.length; t++) {
            var r = c / 2;c += i.encodeWithOffset(a, e, n[t], o + r);
          }
        }(), c;
      }() : n;
    }, d.prototype.decodeParam = function (t, e) {
      return this.decodeParams([t], e)[0];
    }, d.prototype.decodeParams = function (t, e) {
      var n = this.getSolidityTypes(t),
          r = this.getOffsets(t, n);return n.map(function (n, o) {
        return n.decode(e, r[o], t[o], o);
      });
    }, d.prototype.getOffsets = function (t, e) {
      for (var n = e.map(function (e, n) {
        return e.staticPartLength(t[n]);
      }), r = 1; r < n.length; r++) n[r] += n[r - 1];return n.map(function (n, r) {
        return n - e[r].staticPartLength(t[r]);
      });
    }, d.prototype.getSolidityTypes = function (t) {
      var e = this;return t.map(function (t) {
        return e._requireType(t);
      });
    };var m = new d([new o(), new i(), new a(), new s(), new c(), new p(), new u(), new f(), new l()]);e.exports = m;
  }, { "./address": 4, "./bool": 5, "./bytes": 6, "./dynamicbytes": 8, "./formatters": 9, "./int": 10, "./real": 12, "./string": 13, "./uint": 15, "./ureal": 16 }], 8: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputDynamicBytes, this._outputFormatter = r.formatOutputDynamicBytes;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^bytes(\[([0-9]*)\])*$/);
    }, i.prototype.isDynamicType = function () {
      return !0;
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 9: [function (t, e, n) {
    var r = t("bignumber.js"),
        o = t("../utils/utils"),
        i = t("../utils/config"),
        a = t("./param"),
        s = function (t) {
      r.config(i.ETH_BIGNUMBER_ROUNDING_MODE);var e = o.padLeft(o.toTwosComplement(t).toString(16), 64);return new a(e);
    },
        c = function (t) {
      return "1" === new r(t.substr(0, 1), 16).toString(2).substr(0, 1);
    },
        u = function (t) {
      var e = t.staticPart() || "0";return c(e) ? new r(e, 16).minus(new r("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16)).minus(1) : new r(e, 16);
    },
        f = function (t) {
      var e = t.staticPart() || "0";return new r(e, 16);
    };e.exports = { formatInputInt: s, formatInputBytes: function (t) {
        var e = o.toHex(t).substr(2),
            n = Math.floor((e.length + 63) / 64);return e = o.padRight(e, 64 * n), new a(e);
      }, formatInputDynamicBytes: function (t) {
        var e = o.toHex(t).substr(2),
            n = e.length / 2,
            r = Math.floor((e.length + 63) / 64);return e = o.padRight(e, 64 * r), new a(s(n).value + e);
      }, formatInputString: function (t) {
        var e = o.fromUtf8(t).substr(2),
            n = e.length / 2,
            r = Math.floor((e.length + 63) / 64);return e = o.padRight(e, 64 * r), new a(s(n).value + e);
      }, formatInputBool: function (t) {
        return new a("000000000000000000000000000000000000000000000000000000000000000" + (t ? "1" : "0"));
      }, formatInputReal: function (t) {
        return s(new r(t).times(new r(2).pow(128)));
      }, formatOutputInt: u, formatOutputUInt: f, formatOutputReal: function (t) {
        return u(t).dividedBy(new r(2).pow(128));
      }, formatOutputUReal: function (t) {
        return f(t).dividedBy(new r(2).pow(128));
      }, formatOutputBool: function (t) {
        return "0000000000000000000000000000000000000000000000000000000000000001" === t.staticPart();
      }, formatOutputBytes: function (t, e) {
        var n = e.match(/^bytes([0-9]*)/),
            r = parseInt(n[1]);return "0x" + t.staticPart().slice(0, 2 * r);
      }, formatOutputDynamicBytes: function (t) {
        var e = 2 * new r(t.dynamicPart().slice(0, 64), 16).toNumber();return "0x" + t.dynamicPart().substr(64, e);
      }, formatOutputString: function (t) {
        var e = 2 * new r(t.dynamicPart().slice(0, 64), 16).toNumber();return o.toUtf8(t.dynamicPart().substr(64, e));
      }, formatOutputAddress: function (t) {
        var e = t.staticPart();return "0x" + e.slice(e.length - 40, e.length);
      } };
  }, { "../utils/config": 18, "../utils/utils": 20, "./param": 11, "bignumber.js": "bignumber.js" }], 10: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputInt, this._outputFormatter = r.formatOutputInt;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^int([0-9]*)?(\[([0-9]*)\])*$/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 11: [function (t, e, n) {
    var r = t("../utils/utils"),
        o = function (t, e) {
      this.value = t || "", this.offset = e;
    };o.prototype.dynamicPartLength = function () {
      return this.dynamicPart().length / 2;
    }, o.prototype.withOffset = function (t) {
      return new o(this.value, t);
    }, o.prototype.combine = function (t) {
      return new o(this.value + t.value);
    }, o.prototype.isDynamic = function () {
      return void 0 !== this.offset;
    }, o.prototype.offsetAsBytes = function () {
      return this.isDynamic() ? r.padLeft(r.toTwosComplement(this.offset).toString(16), 64) : "";
    }, o.prototype.staticPart = function () {
      return this.isDynamic() ? this.offsetAsBytes() : this.value;
    }, o.prototype.dynamicPart = function () {
      return this.isDynamic() ? this.value : "";
    }, o.prototype.encode = function () {
      return this.staticPart() + this.dynamicPart();
    }, o.encodeList = function (t) {
      var e = 32 * t.length,
          n = t.map(function (t) {
        if (!t.isDynamic()) return t;var n = e;return e += t.dynamicPartLength(), t.withOffset(n);
      });return n.reduce(function (t, e) {
        return t + e.dynamicPart();
      }, n.reduce(function (t, e) {
        return t + e.staticPart();
      }, ""));
    }, e.exports = o;
  }, { "../utils/utils": 20 }], 12: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputReal, this._outputFormatter = r.formatOutputReal;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/real([0-9]*)?(\[([0-9]*)\])?/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 13: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputString, this._outputFormatter = r.formatOutputString;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^string(\[([0-9]*)\])*$/);
    }, i.prototype.isDynamicType = function () {
      return !0;
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 14: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./param"),
        i = function (t) {
      this._inputFormatter = t.inputFormatter, this._outputFormatter = t.outputFormatter;
    };i.prototype.isType = function (t) {
      throw "this method should be overrwritten for type " + t;
    }, i.prototype.staticPartLength = function (t) {
      return (this.nestedTypes(t) || ["[1]"]).map(function (t) {
        return parseInt(t.slice(1, -1), 10) || 1;
      }).reduce(function (t, e) {
        return t * e;
      }, 32);
    }, i.prototype.isDynamicArray = function (t) {
      var e = this.nestedTypes(t);return !!e && !e[e.length - 1].match(/[0-9]{1,}/g);
    }, i.prototype.isStaticArray = function (t) {
      var e = this.nestedTypes(t);return !!e && !!e[e.length - 1].match(/[0-9]{1,}/g);
    }, i.prototype.staticArrayLength = function (t) {
      var e = this.nestedTypes(t);return e ? parseInt(e[e.length - 1].match(/[0-9]{1,}/g) || 1) : 1;
    }, i.prototype.nestedName = function (t) {
      var e = this.nestedTypes(t);return e ? t.substr(0, t.length - e[e.length - 1].length) : t;
    }, i.prototype.isDynamicType = function () {
      return !1;
    }, i.prototype.nestedTypes = function (t) {
      return t.match(/(\[[0-9]*\])/g);
    }, i.prototype.encode = function (t, e) {
      var n = this;return this.isDynamicArray(e) ? function () {
        var o = t.length,
            i = n.nestedName(e),
            a = [];return a.push(r.formatInputInt(o).encode()), t.forEach(function (t) {
          a.push(n.encode(t, i));
        }), a;
      }() : this.isStaticArray(e) ? function () {
        for (var r = n.staticArrayLength(e), o = n.nestedName(e), i = [], a = 0; a < r; a++) i.push(n.encode(t[a], o));return i;
      }() : this._inputFormatter(t, e).encode();
    }, i.prototype.decode = function (t, e, n) {
      var r = this;if (this.isDynamicArray(n)) return function () {
        for (var o = parseInt("0x" + t.substr(2 * e, 64)), i = parseInt("0x" + t.substr(2 * o, 64)), a = o + 32, s = r.nestedName(n), c = r.staticPartLength(s), u = 32 * Math.floor((c + 31) / 32), f = [], l = 0; l < i * u; l += u) f.push(r.decode(t, a + l, s));return f;
      }();if (this.isStaticArray(n)) return function () {
        for (var o = r.staticArrayLength(n), i = e, a = r.nestedName(n), s = r.staticPartLength(a), c = 32 * Math.floor((s + 31) / 32), u = [], f = 0; f < o * c; f += c) u.push(r.decode(t, i + f, a));return u;
      }();if (this.isDynamicType(n)) return function () {
        var i = parseInt("0x" + t.substr(2 * e, 64)),
            a = parseInt("0x" + t.substr(2 * i, 64)),
            s = Math.floor((a + 31) / 32),
            c = new o(t.substr(2 * i, 64 * (1 + s)), 0);return r._outputFormatter(c, n);
      }();var i = this.staticPartLength(n),
          a = new o(t.substr(2 * e, 2 * i));return this._outputFormatter(a, n);
    }, e.exports = i;
  }, { "./formatters": 9, "./param": 11 }], 15: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputInt, this._outputFormatter = r.formatOutputUInt;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^uint([0-9]*)?(\[([0-9]*)\])*$/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 16: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./type"),
        i = function () {
      this._inputFormatter = r.formatInputReal, this._outputFormatter = r.formatOutputUReal;
    };(i.prototype = new o({})).constructor = i, i.prototype.isType = function (t) {
      return !!t.match(/^ureal([0-9]*)?(\[([0-9]*)\])*$/);
    }, e.exports = i;
  }, { "./formatters": 9, "./type": 14 }], 17: [function (t, e, n) {
    "use strict";
    "undefined" == typeof XMLHttpRequest ? n.XMLHttpRequest = {} : n.XMLHttpRequest = XMLHttpRequest;
  }, {}], 18: [function (t, e, n) {
    var r = t("bignumber.js"),
        o = ["wei", "kwei", "Mwei", "Gwei", "szabo", "finney", "femtoether", "picoether", "nanoether", "microether", "milliether", "nano", "micro", "milli", "ether", "grand", "Mether", "Gether", "Tether", "Pether", "Eether", "Zether", "Yether", "Nether", "Dether", "Vether", "Uether"];e.exports = { ETH_PADDING: 32, ETH_SIGNATURE_LENGTH: 4, ETH_UNITS: o, ETH_BIGNUMBER_ROUNDING_MODE: { ROUNDING_MODE: r.ROUND_DOWN }, ETH_POLLING_TIMEOUT: 500, defaultBlock: "latest", defaultAccount: void 0 };
  }, { "bignumber.js": "bignumber.js" }], 19: [function (t, e, n) {
    var r = t("crypto-js"),
        o = t("crypto-js/sha3");e.exports = function (t, e) {
      return e && "hex" === e.encoding && (t.length > 2 && "0x" === t.substr(0, 2) && (t = t.substr(2)), t = r.enc.Hex.parse(t)), o(t, { outputLength: 256 }).toString();
    };
  }, { "crypto-js": 58, "crypto-js/sha3": 79 }], 20: [function (t, e, n) {
    var r = t("bignumber.js"),
        o = t("./sha3.js"),
        i = t("utf8"),
        a = { noether: "0", wei: "1", kwei: "1000", Kwei: "1000", babbage: "1000", femtoether: "1000", mwei: "1000000", Mwei: "1000000", lovelace: "1000000", picoether: "1000000", gwei: "1000000000", Gwei: "1000000000", shannon: "1000000000", nanoether: "1000000000", nano: "1000000000", szabo: "1000000000000", microether: "1000000000000", micro: "1000000000000", finney: "1000000000000000", milliether: "1000000000000000", milli: "1000000000000000", ether: "1000000000000000000", kether: "1000000000000000000000", grand: "1000000000000000000000", mether: "1000000000000000000000000", gether: "1000000000000000000000000000", tether: "1000000000000000000000000000000" },
        s = function (t, e, n) {
      return new Array(e - t.length + 1).join(n || "0") + t;
    },
        c = function (t) {
      t = i.encode(t);for (var e = "", n = 0; n < t.length; n++) {
        var r = t.charCodeAt(n);if (0 === r) break;var o = r.toString(16);e += o.length < 2 ? "0" + o : o;
      }return "0x" + e;
    },
        u = function (t) {
      for (var e = "", n = 0; n < t.length; n++) {
        var r = t.charCodeAt(n).toString(16);e += r.length < 2 ? "0" + r : r;
      }return "0x" + e;
    },
        f = function (t) {
      var e = h(t),
          n = e.toString(16);return e.lessThan(0) ? "-0x" + n.substr(1) : "0x" + n;
    },
        l = function (t) {
      if (v(t)) return f(+t);if (y(t)) return f(t);if ("object" == typeof t) return c(JSON.stringify(t));if (g(t)) {
        if (0 === t.indexOf("-0x")) return f(t);if (0 === t.indexOf("0x")) return t;if (!isFinite(t)) return u(t);
      }return f(t);
    },
        p = function (t) {
      t = t ? t.toLowerCase() : "ether";var e = a[t];if (void 0 === e) throw new Error("This unit doesn't exists, please use the one of the following units" + JSON.stringify(a, null, 2));return new r(e, 10);
    },
        h = function (t) {
      return t = t || 0, y(t) ? t : !g(t) || 0 !== t.indexOf("0x") && 0 !== t.indexOf("-0x") ? new r(t.toString(10), 10) : new r(t.replace("0x", ""), 16);
    },
        d = function (t) {
      return (/^0x[0-9a-f]{40}$/i.test(t)
      );
    },
        m = function (t) {
      t = t.replace("0x", "");for (var e = o(t.toLowerCase()), n = 0; n < 40; n++) if (parseInt(e[n], 16) > 7 && t[n].toUpperCase() !== t[n] || parseInt(e[n], 16) <= 7 && t[n].toLowerCase() !== t[n]) return !1;return !0;
    },
        y = function (t) {
      return t instanceof r || t && t.constructor && "BigNumber" === t.constructor.name;
    },
        g = function (t) {
      return "string" == typeof t || t && t.constructor && "String" === t.constructor.name;
    },
        v = function (t) {
      return "boolean" == typeof t;
    };e.exports = { padLeft: s, padRight: function (t, e, n) {
        return t + new Array(e - t.length + 1).join(n || "0");
      }, toHex: l, toDecimal: function (t) {
        return h(t).toNumber();
      }, fromDecimal: f, toUtf8: function (t) {
        var e = "",
            n = 0,
            r = t.length;for ("0x" === t.substring(0, 2) && (n = 2); n < r; n += 2) {
          var o = parseInt(t.substr(n, 2), 16);if (0 === o) break;e += String.fromCharCode(o);
        }return i.decode(e);
      }, toAscii: function (t) {
        var e = "",
            n = 0,
            r = t.length;for ("0x" === t.substring(0, 2) && (n = 2); n < r; n += 2) {
          var o = parseInt(t.substr(n, 2), 16);e += String.fromCharCode(o);
        }return e;
      }, fromUtf8: c, fromAscii: u, transformToFullName: function (t) {
        if (-1 !== t.name.indexOf("(")) return t.name;var e = t.inputs.map(function (t) {
          return t.type;
        }).join();return t.name + "(" + e + ")";
      }, extractDisplayName: function (t) {
        var e = t.indexOf("(");return -1 !== e ? t.substr(0, e) : t;
      }, extractTypeName: function (t) {
        var e = t.indexOf("(");return -1 !== e ? t.substr(e + 1, t.length - 1 - (e + 1)).replace(" ", "") : "";
      }, toWei: function (t, e) {
        var n = h(t).times(p(e));return y(t) ? n : n.toString(10);
      }, fromWei: function (t, e) {
        var n = h(t).dividedBy(p(e));return y(t) ? n : n.toString(10);
      }, toBigNumber: h, toTwosComplement: function (t) {
        var e = h(t).round();return e.lessThan(0) ? new r("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16).plus(e).plus(1) : e;
      }, toAddress: function (t) {
        return d(t) ? t : /^[0-9a-f]{40}$/.test(t) ? "0x" + t : "0x" + s(l(t).substr(2), 40);
      }, isBigNumber: y, isStrictAddress: d, isAddress: function (t) {
        return !!/^(0x)?[0-9a-f]{40}$/i.test(t) && (!(!/^(0x)?[0-9a-f]{40}$/.test(t) && !/^(0x)?[0-9A-F]{40}$/.test(t)) || m(t));
      }, isChecksumAddress: m, toChecksumAddress: function (t) {
        if (void 0 === t) return "";t = t.toLowerCase().replace("0x", "");for (var e = o(t), n = "0x", r = 0; r < t.length; r++) parseInt(e[r], 16) > 7 ? n += t[r].toUpperCase() : n += t[r];return n;
      }, isFunction: function (t) {
        return "function" == typeof t;
      }, isString: g, isObject: function (t) {
        return null !== t && !(t instanceof Array) && "object" == typeof t;
      }, isBoolean: v, isArray: function (t) {
        return t instanceof Array;
      }, isJson: function (t) {
        try {
          return !!JSON.parse(t);
        } catch (t) {
          return !1;
        }
      }, isBloom: function (t) {
        return !(!/^(0x)?[0-9a-f]{512}$/i.test(t) || !/^(0x)?[0-9a-f]{512}$/.test(t) && !/^(0x)?[0-9A-F]{512}$/.test(t));
      }, isTopic: function (t) {
        return !(!/^(0x)?[0-9a-f]{64}$/i.test(t) || !/^(0x)?[0-9a-f]{64}$/.test(t) && !/^(0x)?[0-9A-F]{64}$/.test(t));
      } };
  }, { "./sha3.js": 19, "bignumber.js": "bignumber.js", utf8: 84 }], 21: [function (t, e, n) {
    e.exports = { version: "0.20.1" };
  }, {}], 22: [function (t, e, n) {
    function r(t) {
      this._requestManager = new o(t), this.currentProvider = t, this.eth = new a(this), this.db = new s(this), this.shh = new c(this), this.net = new u(this), this.personal = new f(this), this.bzz = new l(this), this.settings = new p(), this.version = { api: h.version }, this.providers = { HttpProvider: b, IpcProvider: _ }, this._extend = y(this), this._extend({ properties: x() });
    }var o = t("./web3/requestmanager"),
        i = t("./web3/iban"),
        a = t("./web3/methods/eth"),
        s = t("./web3/methods/db"),
        c = t("./web3/methods/shh"),
        u = t("./web3/methods/net"),
        f = t("./web3/methods/personal"),
        l = t("./web3/methods/swarm"),
        p = t("./web3/settings"),
        h = t("./version.json"),
        d = t("./utils/utils"),
        m = t("./utils/sha3"),
        y = t("./web3/extend"),
        g = t("./web3/batch"),
        v = t("./web3/property"),
        b = t("./web3/httpprovider"),
        _ = t("./web3/ipcprovider"),
        w = t("bignumber.js");r.providers = { HttpProvider: b, IpcProvider: _ }, r.prototype.setProvider = function (t) {
      this._requestManager.setProvider(t), this.currentProvider = t;
    }, r.prototype.reset = function (t) {
      this._requestManager.reset(t), this.settings = new p();
    }, r.prototype.BigNumber = w, r.prototype.toHex = d.toHex, r.prototype.toAscii = d.toAscii, r.prototype.toUtf8 = d.toUtf8, r.prototype.fromAscii = d.fromAscii, r.prototype.fromUtf8 = d.fromUtf8, r.prototype.toDecimal = d.toDecimal, r.prototype.fromDecimal = d.fromDecimal, r.prototype.toBigNumber = d.toBigNumber, r.prototype.toWei = d.toWei, r.prototype.fromWei = d.fromWei, r.prototype.isAddress = d.isAddress, r.prototype.isChecksumAddress = d.isChecksumAddress, r.prototype.toChecksumAddress = d.toChecksumAddress, r.prototype.isIBAN = d.isIBAN, r.prototype.padLeft = d.padLeft, r.prototype.padRight = d.padRight, r.prototype.sha3 = function (t, e) {
      return "0x" + m(t, e);
    }, r.prototype.fromICAP = function (t) {
      return new i(t).address();
    };var x = function () {
      return [new v({ name: "version.node", getter: "web3_clientVersion" }), new v({ name: "version.network", getter: "net_version", inputFormatter: d.toDecimal }), new v({ name: "version.ethereum", getter: "eth_protocolVersion", inputFormatter: d.toDecimal }), new v({ name: "version.whisper", getter: "shh_version", inputFormatter: d.toDecimal })];
    };r.prototype.isConnected = function () {
      return this.currentProvider && this.currentProvider.isConnected();
    }, r.prototype.createBatch = function () {
      return new g(this);
    }, e.exports = r;
  }, { "./utils/sha3": 19, "./utils/utils": 20, "./version.json": 21, "./web3/batch": 24, "./web3/extend": 28, "./web3/httpprovider": 32, "./web3/iban": 33, "./web3/ipcprovider": 34, "./web3/methods/db": 37, "./web3/methods/eth": 38, "./web3/methods/net": 39, "./web3/methods/personal": 40, "./web3/methods/shh": 41, "./web3/methods/swarm": 42, "./web3/property": 45, "./web3/requestmanager": 46, "./web3/settings": 47, "bignumber.js": "bignumber.js" }], 23: [function (t, e, n) {
    var r = t("../utils/sha3"),
        o = t("./event"),
        i = t("./formatters"),
        a = t("../utils/utils"),
        s = t("./filter"),
        c = t("./methods/watches"),
        u = function (t, e, n) {
      this._requestManager = t, this._json = e, this._address = n;
    };u.prototype.encode = function (t) {
      t = t || {};var e = {};return ["fromBlock", "toBlock"].filter(function (e) {
        return void 0 !== t[e];
      }).forEach(function (n) {
        e[n] = i.inputBlockNumberFormatter(t[n]);
      }), e.address = this._address, e;
    }, u.prototype.decode = function (t) {
      t.data = t.data || "", t.topics = t.topics || [];var e = t.topics[0].slice(2),
          n = this._json.filter(function (t) {
        return e === r(a.transformToFullName(t));
      })[0];return n ? new o(this._requestManager, n, this._address).decode(t) : (console.warn("cannot find event for log"), t);
    }, u.prototype.execute = function (t, e) {
      a.isFunction(arguments[arguments.length - 1]) && (e = arguments[arguments.length - 1], 1 === arguments.length && (t = null));var n = this.encode(t),
          r = this.decode.bind(this);return new s(n, "eth", this._requestManager, c.eth(), r, e);
    }, u.prototype.attachToContract = function (t) {
      var e = this.execute.bind(this);t.allEvents = e;
    }, e.exports = u;
  }, { "../utils/sha3": 19, "../utils/utils": 20, "./event": 27, "./filter": 29, "./formatters": 30, "./methods/watches": 43 }], 24: [function (t, e, n) {
    var r = t("./jsonrpc"),
        o = t("./errors"),
        i = function (t) {
      this.requestManager = t._requestManager, this.requests = [];
    };i.prototype.add = function (t) {
      this.requests.push(t);
    }, i.prototype.execute = function () {
      var t = this.requests;this.requestManager.sendBatch(t, function (e, n) {
        n = n || [], t.map(function (t, e) {
          return n[e] || {};
        }).forEach(function (e, n) {
          if (t[n].callback) {
            if (!r.isValidResponse(e)) return t[n].callback(o.InvalidResponse(e));t[n].callback(null, t[n].format ? t[n].format(e.result) : e.result);
          }
        });
      });
    }, e.exports = i;
  }, { "./errors": 26, "./jsonrpc": 35 }], 25: [function (t, e, n) {
    var r = t("../utils/utils"),
        o = t("../solidity/coder"),
        i = t("./event"),
        a = t("./function"),
        s = t("./allevents"),
        c = function (t, e) {
      return t.filter(function (t) {
        return "constructor" === t.type && t.inputs.length === e.length;
      }).map(function (t) {
        return t.inputs.map(function (t) {
          return t.type;
        });
      }).map(function (t) {
        return o.encodeParams(t, e);
      })[0] || "";
    },
        u = function (t) {
      t.abi.filter(function (t) {
        return "function" === t.type;
      }).map(function (e) {
        return new a(t._eth, e, t.address);
      }).forEach(function (e) {
        e.attachToContract(t);
      });
    },
        f = function (t) {
      var e = t.abi.filter(function (t) {
        return "event" === t.type;
      });new s(t._eth._requestManager, e, t.address).attachToContract(t), e.map(function (e) {
        return new i(t._eth._requestManager, e, t.address);
      }).forEach(function (e) {
        e.attachToContract(t);
      });
    },
        l = function (t, e) {
      var n = 0,
          r = !1,
          o = t._eth.filter("latest", function (i) {
        if (!i && !r) if (++n > 50) {
          if (o.stopWatching(function () {}), r = !0, !e) throw new Error("Contract transaction couldn't be found after 50 blocks");e(new Error("Contract transaction couldn't be found after 50 blocks"));
        } else t._eth.getTransactionReceipt(t.transactionHash, function (n, i) {
          i && !r && t._eth.getCode(i.contractAddress, function (n, a) {
            if (!r && a) if (o.stopWatching(function () {}), r = !0, a.length > 3) t.address = i.contractAddress, u(t), f(t), e && e(null, t);else {
              if (!e) throw new Error("The contract code couldn't be stored, please check your gas amount.");e(new Error("The contract code couldn't be stored, please check your gas amount."));
            }
          });
        });
      });
    },
        p = function (t, e) {
      this.eth = t, this.abi = e, this.new = function () {
        var t,
            n = new h(this.eth, this.abi),
            o = {},
            i = Array.prototype.slice.call(arguments);r.isFunction(i[i.length - 1]) && (t = i.pop());var a = i[i.length - 1];if (r.isObject(a) && !r.isArray(a) && (o = i.pop()), o.value > 0 && !(e.filter(function (t) {
          return "constructor" === t.type && t.inputs.length === i.length;
        })[0] || {}).payable) throw new Error("Cannot send value to non-payable constructor");var s = c(this.abi, i);if (o.data += s, t) this.eth.sendTransaction(o, function (e, r) {
          e ? t(e) : (n.transactionHash = r, t(null, n), l(n, t));
        });else {
          var u = this.eth.sendTransaction(o);n.transactionHash = u, l(n);
        }return n;
      }, this.new.getData = this.getData.bind(this);
    };p.prototype.at = function (t, e) {
      var n = new h(this.eth, this.abi, t);return u(n), f(n), e && e(null, n), n;
    }, p.prototype.getData = function () {
      var t = {},
          e = Array.prototype.slice.call(arguments),
          n = e[e.length - 1];r.isObject(n) && !r.isArray(n) && (t = e.pop());var o = c(this.abi, e);return t.data += o, t.data;
    };var h = function (t, e, n) {
      this._eth = t, this.transactionHash = null, this.address = n, this.abi = e;
    };e.exports = p;
  }, { "../solidity/coder": 7, "../utils/utils": 20, "./allevents": 23, "./event": 27, "./function": 31 }], 26: [function (t, e, n) {
    e.exports = { InvalidNumberOfSolidityArgs: function () {
        return new Error("Invalid number of arguments to Solidity function");
      }, InvalidNumberOfRPCParams: function () {
        return new Error("Invalid number of input parameters to RPC method");
      }, InvalidConnection: function (t) {
        return new Error("CONNECTION ERROR: Couldn't connect to node " + t + ".");
      }, InvalidProvider: function () {
        return new Error("Provider not set or invalid");
      }, InvalidResponse: function (t) {
        var e = t && t.error && t.error.message ? t.error.message : "Invalid JSON RPC response: " + JSON.stringify(t);return new Error(e);
      }, ConnectionTimeout: function (t) {
        return new Error("CONNECTION TIMEOUT: timeout of " + t + " ms achived");
      } };
  }, {}], 27: [function (t, e, n) {
    var r = t("../utils/utils"),
        o = t("../solidity/coder"),
        i = t("./formatters"),
        a = t("../utils/sha3"),
        s = t("./filter"),
        c = t("./methods/watches"),
        u = function (t, e, n) {
      this._requestManager = t, this._params = e.inputs, this._name = r.transformToFullName(e), this._address = n, this._anonymous = e.anonymous;
    };u.prototype.types = function (t) {
      return this._params.filter(function (e) {
        return e.indexed === t;
      }).map(function (t) {
        return t.type;
      });
    }, u.prototype.displayName = function () {
      return r.extractDisplayName(this._name);
    }, u.prototype.typeName = function () {
      return r.extractTypeName(this._name);
    }, u.prototype.signature = function () {
      return a(this._name);
    }, u.prototype.encode = function (t, e) {
      t = t || {}, e = e || {};var n = {};["fromBlock", "toBlock"].filter(function (t) {
        return void 0 !== e[t];
      }).forEach(function (t) {
        n[t] = i.inputBlockNumberFormatter(e[t]);
      }), n.topics = [], n.address = this._address, this._anonymous || n.topics.push("0x" + this.signature());var a = this._params.filter(function (t) {
        return !0 === t.indexed;
      }).map(function (e) {
        var n = t[e.name];return void 0 === n || null === n ? null : r.isArray(n) ? n.map(function (t) {
          return "0x" + o.encodeParam(e.type, t);
        }) : "0x" + o.encodeParam(e.type, n);
      });return n.topics = n.topics.concat(a), n;
    }, u.prototype.decode = function (t) {
      t.data = t.data || "", t.topics = t.topics || [];var e = (this._anonymous ? t.topics : t.topics.slice(1)).map(function (t) {
        return t.slice(2);
      }).join(""),
          n = o.decodeParams(this.types(!0), e),
          r = t.data.slice(2),
          a = o.decodeParams(this.types(!1), r),
          s = i.outputLogFormatter(t);return s.event = this.displayName(), s.address = t.address, s.args = this._params.reduce(function (t, e) {
        return t[e.name] = e.indexed ? n.shift() : a.shift(), t;
      }, {}), delete s.data, delete s.topics, s;
    }, u.prototype.execute = function (t, e, n) {
      r.isFunction(arguments[arguments.length - 1]) && (n = arguments[arguments.length - 1], 2 === arguments.length && (e = null), 1 === arguments.length && (e = null, t = {}));var o = this.encode(t, e),
          i = this.decode.bind(this);return new s(o, "eth", this._requestManager, c.eth(), i, n);
    }, u.prototype.attachToContract = function (t) {
      var e = this.execute.bind(this),
          n = this.displayName();t[n] || (t[n] = e), t[n][this.typeName()] = this.execute.bind(this, t);
    }, e.exports = u;
  }, { "../solidity/coder": 7, "../utils/sha3": 19, "../utils/utils": 20, "./filter": 29, "./formatters": 30, "./methods/watches": 43 }], 28: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("./../utils/utils"),
        i = t("./method"),
        a = t("./property");e.exports = function (t) {
      var e = function (e) {
        var n;e.property ? (t[e.property] || (t[e.property] = {}), n = t[e.property]) : n = t, e.methods && e.methods.forEach(function (e) {
          e.attachToObject(n), e.setRequestManager(t._requestManager);
        }), e.properties && e.properties.forEach(function (e) {
          e.attachToObject(n), e.setRequestManager(t._requestManager);
        });
      };return e.formatters = r, e.utils = o, e.Method = i, e.Property = a, e;
    };
  }, { "./../utils/utils": 20, "./formatters": 30, "./method": 36, "./property": 45 }], 29: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("../utils/utils"),
        i = function (t) {
      return null === t || void 0 === t ? null : 0 === (t = String(t)).indexOf("0x") ? t : o.fromUtf8(t);
    },
        a = function (t, e) {
      if (o.isString(t)) return t;switch (t = t || {}, e) {case "eth":
          return t.topics = t.topics || [], t.topics = t.topics.map(function (t) {
            return o.isArray(t) ? t.map(i) : i(t);
          }), { topics: t.topics, from: t.from, to: t.to, address: t.address, fromBlock: r.inputBlockNumberFormatter(t.fromBlock), toBlock: r.inputBlockNumberFormatter(t.toBlock) };case "shh":
          return t;}
    },
        s = function (t, e) {
      o.isString(t.options) || t.get(function (t, n) {
        t && e(t), o.isArray(n) && n.forEach(function (t) {
          e(null, t);
        });
      });
    },
        c = function (t) {
      t.requestManager.startPolling({ method: t.implementation.poll.call, params: [t.filterId] }, t.filterId, function (e, n) {
        if (e) return t.callbacks.forEach(function (t) {
          t(e);
        });o.isArray(n) && n.forEach(function (e) {
          e = t.formatter ? t.formatter(e) : e, t.callbacks.forEach(function (t) {
            t(null, e);
          });
        });
      }, t.stopWatching.bind(t));
    },
        u = function (t, e, n, r, o, i, u) {
      var f = this,
          l = {};return r.forEach(function (t) {
        t.setRequestManager(n), t.attachToObject(l);
      }), this.requestManager = n, this.options = a(t, e), this.implementation = l, this.filterId = null, this.callbacks = [], this.getLogsCallbacks = [], this.pollFilters = [], this.formatter = o, this.implementation.newFilter(this.options, function (t, e) {
        if (t) f.callbacks.forEach(function (e) {
          e(t);
        }), "function" == typeof u && u(t);else if (f.filterId = e, f.getLogsCallbacks.forEach(function (t) {
          f.get(t);
        }), f.getLogsCallbacks = [], f.callbacks.forEach(function (t) {
          s(f, t);
        }), f.callbacks.length > 0 && c(f), "function" == typeof i) return f.watch(i);
      }), this;
    };u.prototype.watch = function (t) {
      return this.callbacks.push(t), this.filterId && (s(this, t), c(this)), this;
    }, u.prototype.stopWatching = function (t) {
      if (this.requestManager.stopPolling(this.filterId), this.callbacks = [], !t) return this.implementation.uninstallFilter(this.filterId);this.implementation.uninstallFilter(this.filterId, t);
    }, u.prototype.get = function (t) {
      var e = this;if (!o.isFunction(t)) {
        if (null === this.filterId) throw new Error("Filter ID Error: filter().get() can't be chained synchronous, please provide a callback for the get() method.");return this.implementation.getLogs(this.filterId).map(function (t) {
          return e.formatter ? e.formatter(t) : t;
        });
      }return null === this.filterId ? this.getLogsCallbacks.push(t) : this.implementation.getLogs(this.filterId, function (n, r) {
        n ? t(n) : t(null, r.map(function (t) {
          return e.formatter ? e.formatter(t) : t;
        }));
      }), this;
    }, e.exports = u;
  }, { "../utils/utils": 20, "./formatters": 30 }], 30: [function (t, e, n) {
    "use strict";
    var r = t("../utils/utils"),
        o = t("../utils/config"),
        i = t("./iban"),
        a = function (t) {
      return "latest" === t || "pending" === t || "earliest" === t;
    },
        s = function (t) {
      if (void 0 !== t) return a(t) ? t : r.toHex(t);
    },
        c = function (t) {
      return null !== t.blockNumber && (t.blockNumber = r.toDecimal(t.blockNumber)), null !== t.transactionIndex && (t.transactionIndex = r.toDecimal(t.transactionIndex)), t.nonce = r.toDecimal(t.nonce), t.gas = r.toDecimal(t.gas), t.gasPrice = r.toBigNumber(t.gasPrice), t.value = r.toBigNumber(t.value), t;
    },
        u = function (t) {
      return t.blockNumber && (t.blockNumber = r.toDecimal(t.blockNumber)), t.transactionIndex && (t.transactionIndex = r.toDecimal(t.transactionIndex)), t.logIndex && (t.logIndex = r.toDecimal(t.logIndex)), t;
    },
        f = function (t) {
      var e = new i(t);if (e.isValid() && e.isDirect()) return "0x" + e.address();if (r.isStrictAddress(t)) return t;if (r.isAddress(t)) return "0x" + t;throw new Error("invalid address");
    };e.exports = { inputDefaultBlockNumberFormatter: function (t) {
        return void 0 === t ? o.defaultBlock : s(t);
      }, inputBlockNumberFormatter: s, inputCallFormatter: function (t) {
        return t.from = t.from || o.defaultAccount, t.from && (t.from = f(t.from)), t.to && (t.to = f(t.to)), ["gasPrice", "gas", "value", "nonce"].filter(function (e) {
          return void 0 !== t[e];
        }).forEach(function (e) {
          t[e] = r.fromDecimal(t[e]);
        }), t;
      }, inputTransactionFormatter: function (t) {
        return t.from = t.from || o.defaultAccount, t.from = f(t.from), t.to && (t.to = f(t.to)), ["gasPrice", "gas", "value", "nonce"].filter(function (e) {
          return void 0 !== t[e];
        }).forEach(function (e) {
          t[e] = r.fromDecimal(t[e]);
        }), t;
      }, inputAddressFormatter: f, inputPostFormatter: function (t) {
        return t.ttl = r.fromDecimal(t.ttl), t.workToProve = r.fromDecimal(t.workToProve), t.priority = r.fromDecimal(t.priority), r.isArray(t.topics) || (t.topics = t.topics ? [t.topics] : []), t.topics = t.topics.map(function (t) {
          return 0 === t.indexOf("0x") ? t : r.fromUtf8(t);
        }), t;
      }, outputBigNumberFormatter: function (t) {
        return r.toBigNumber(t);
      }, outputTransactionFormatter: c, outputTransactionReceiptFormatter: function (t) {
        return null !== t.blockNumber && (t.blockNumber = r.toDecimal(t.blockNumber)), null !== t.transactionIndex && (t.transactionIndex = r.toDecimal(t.transactionIndex)), t.cumulativeGasUsed = r.toDecimal(t.cumulativeGasUsed), t.gasUsed = r.toDecimal(t.gasUsed), r.isArray(t.logs) && (t.logs = t.logs.map(function (t) {
          return u(t);
        })), t;
      }, outputBlockFormatter: function (t) {
        return t.gasLimit = r.toDecimal(t.gasLimit), t.gasUsed = r.toDecimal(t.gasUsed), t.size = r.toDecimal(t.size), t.timestamp = r.toDecimal(t.timestamp), null !== t.number && (t.number = r.toDecimal(t.number)), t.difficulty = r.toBigNumber(t.difficulty), t.totalDifficulty = r.toBigNumber(t.totalDifficulty), r.isArray(t.transactions) && t.transactions.forEach(function (t) {
          if (!r.isString(t)) return c(t);
        }), t;
      }, outputLogFormatter: u, outputPostFormatter: function (t) {
        return t.expiry = r.toDecimal(t.expiry), t.sent = r.toDecimal(t.sent), t.ttl = r.toDecimal(t.ttl), t.workProved = r.toDecimal(t.workProved), t.topics || (t.topics = []), t.topics = t.topics.map(function (t) {
          return r.toAscii(t);
        }), t;
      }, outputSyncingFormatter: function (t) {
        return t ? (t.startingBlock = r.toDecimal(t.startingBlock), t.currentBlock = r.toDecimal(t.currentBlock), t.highestBlock = r.toDecimal(t.highestBlock), t.knownStates && (t.knownStates = r.toDecimal(t.knownStates), t.pulledStates = r.toDecimal(t.pulledStates)), t) : t;
      } };
  }, { "../utils/config": 18, "../utils/utils": 20, "./iban": 33 }], 31: [function (t, e, n) {
    var r = t("../solidity/coder"),
        o = t("../utils/utils"),
        i = t("./errors"),
        a = t("./formatters"),
        s = t("../utils/sha3"),
        c = function (t, e, n) {
      this._eth = t, this._inputTypes = e.inputs.map(function (t) {
        return t.type;
      }), this._outputTypes = e.outputs.map(function (t) {
        return t.type;
      }), this._constant = e.constant, this._payable = e.payable, this._name = o.transformToFullName(e), this._address = n;
    };c.prototype.extractCallback = function (t) {
      if (o.isFunction(t[t.length - 1])) return t.pop();
    }, c.prototype.extractDefaultBlock = function (t) {
      if (t.length > this._inputTypes.length && !o.isObject(t[t.length - 1])) return a.inputDefaultBlockNumberFormatter(t.pop());
    }, c.prototype.validateArgs = function (t) {
      if (t.filter(function (t) {
        return !(!0 === o.isObject(t) && !1 === o.isArray(t) && !1 === o.isBigNumber(t));
      }).length !== this._inputTypes.length) throw i.InvalidNumberOfSolidityArgs();
    }, c.prototype.toPayload = function (t) {
      var e = {};return t.length > this._inputTypes.length && o.isObject(t[t.length - 1]) && (e = t[t.length - 1]), this.validateArgs(t), e.to = this._address, e.data = "0x" + this.signature() + r.encodeParams(this._inputTypes, t), e;
    }, c.prototype.signature = function () {
      return s(this._name).slice(0, 8);
    }, c.prototype.unpackOutput = function (t) {
      if (t) {
        t = t.length >= 2 ? t.slice(2) : t;var e = r.decodeParams(this._outputTypes, t);return 1 === e.length ? e[0] : e;
      }
    }, c.prototype.call = function () {
      var t = Array.prototype.slice.call(arguments).filter(function (t) {
        return void 0 !== t;
      }),
          e = this.extractCallback(t),
          n = this.extractDefaultBlock(t),
          r = this.toPayload(t);if (!e) {
        var o = this._eth.call(r, n);return this.unpackOutput(o);
      }var i = this;this._eth.call(r, n, function (t, n) {
        if (t) return e(t, null);var r = null;try {
          r = i.unpackOutput(n);
        } catch (e) {
          t = e;
        }e(t, r);
      });
    }, c.prototype.sendTransaction = function () {
      var t = Array.prototype.slice.call(arguments).filter(function (t) {
        return void 0 !== t;
      }),
          e = this.extractCallback(t),
          n = this.toPayload(t);if (n.value > 0 && !this._payable) throw new Error("Cannot send value to non-payable function");if (!e) return this._eth.sendTransaction(n);this._eth.sendTransaction(n, e);
    }, c.prototype.estimateGas = function () {
      var t = Array.prototype.slice.call(arguments),
          e = this.extractCallback(t),
          n = this.toPayload(t);if (!e) return this._eth.estimateGas(n);this._eth.estimateGas(n, e);
    }, c.prototype.getData = function () {
      var t = Array.prototype.slice.call(arguments);return this.toPayload(t).data;
    }, c.prototype.displayName = function () {
      return o.extractDisplayName(this._name);
    }, c.prototype.typeName = function () {
      return o.extractTypeName(this._name);
    }, c.prototype.request = function () {
      var t = Array.prototype.slice.call(arguments),
          e = this.extractCallback(t),
          n = this.toPayload(t),
          r = this.unpackOutput.bind(this);return { method: this._constant ? "eth_call" : "eth_sendTransaction", callback: e, params: [n], format: r };
    }, c.prototype.execute = function () {
      return !this._constant ? this.sendTransaction.apply(this, Array.prototype.slice.call(arguments)) : this.call.apply(this, Array.prototype.slice.call(arguments));
    }, c.prototype.attachToContract = function (t) {
      var e = this.execute.bind(this);e.request = this.request.bind(this), e.call = this.call.bind(this), e.sendTransaction = this.sendTransaction.bind(this), e.estimateGas = this.estimateGas.bind(this), e.getData = this.getData.bind(this);var n = this.displayName();t[n] || (t[n] = e), t[n][this.typeName()] = e;
    }, e.exports = c;
  }, { "../solidity/coder": 7, "../utils/sha3": 19, "../utils/utils": 20, "./errors": 26, "./formatters": 30 }], 32: [function (t, e, n) {
    var r = t("./errors");"undefined" != typeof window && window.XMLHttpRequest ? XMLHttpRequest = window.XMLHttpRequest : XMLHttpRequest = t("xmlhttprequest").XMLHttpRequest;var o = t("xhr2"),
        i = function (t, e, n, r) {
      this.host = t || "http://localhost:8545", this.timeout = e || 0, this.user = n, this.password = r;
    };i.prototype.prepareRequest = function (t) {
      var e;if (t ? (e = new o()).timeout = this.timeout : e = new XMLHttpRequest(), e.open("POST", this.host, t), this.user && this.password) {
        var n = "Basic " + new Buffer(this.user + ":" + this.password).toString("base64");e.setRequestHeader("Authorization", n);
      }return e.setRequestHeader("Content-Type", "application/json"), e;
    }, i.prototype.send = function (t) {
      var e = this.prepareRequest(!1);try {
        e.send(JSON.stringify(t));
      } catch (t) {
        throw r.InvalidConnection(this.host);
      }var n = e.responseText;try {
        n = JSON.parse(n);
      } catch (t) {
        throw r.InvalidResponse(e.responseText);
      }return n;
    }, i.prototype.sendAsync = function (t, e) {
      var n = this.prepareRequest(!0);n.onreadystatechange = function () {
        if (4 === n.readyState && 1 !== n.timeout) {
          var t = n.responseText,
              o = null;try {
            t = JSON.parse(t);
          } catch (t) {
            o = r.InvalidResponse(n.responseText);
          }e(o, t);
        }
      }, n.ontimeout = function () {
        e(r.ConnectionTimeout(this.timeout));
      };try {
        n.send(JSON.stringify(t));
      } catch (t) {
        e(r.InvalidConnection(this.host));
      }
    }, i.prototype.isConnected = function () {
      try {
        return this.send({ id: 9999999999, jsonrpc: "2.0", method: "net_listening", params: [] }), !0;
      } catch (t) {
        return !1;
      }
    }, e.exports = i;
  }, { "./errors": 26, xhr2: 85, xmlhttprequest: 17 }], 33: [function (t, e, n) {
    var r = t("bignumber.js"),
        o = function (t, e) {
      for (var n = t; n.length < 2 * e;) n = "0" + n;return n;
    },
        i = function (t) {
      var e = "A".charCodeAt(0),
          n = "Z".charCodeAt(0);return t = t.toUpperCase(), (t = t.substr(4) + t.substr(0, 4)).split("").map(function (t) {
        var r = t.charCodeAt(0);return r >= e && r <= n ? r - e + 10 : t;
      }).join("");
    },
        a = function (t) {
      for (var e, n = t; n.length > 2;) e = n.slice(0, 9), n = parseInt(e, 10) % 97 + n.slice(e.length);return parseInt(n, 10) % 97;
    },
        s = function (t) {
      this._iban = t;
    };s.fromAddress = function (t) {
      var e = new r(t, 16).toString(36),
          n = o(e, 15);return s.fromBban(n.toUpperCase());
    }, s.fromBban = function (t) {
      var e = ("0" + (98 - a(i("XE00" + t)))).slice(-2);return new s("XE" + e + t);
    }, s.createIndirect = function (t) {
      return s.fromBban("ETH" + t.institution + t.identifier);
    }, s.isValid = function (t) {
      return new s(t).isValid();
    }, s.prototype.isValid = function () {
      return (/^XE[0-9]{2}(ETH[0-9A-Z]{13}|[0-9A-Z]{30,31})$/.test(this._iban) && 1 === a(i(this._iban))
      );
    }, s.prototype.isDirect = function () {
      return 34 === this._iban.length || 35 === this._iban.length;
    }, s.prototype.isIndirect = function () {
      return 20 === this._iban.length;
    }, s.prototype.checksum = function () {
      return this._iban.substr(2, 2);
    }, s.prototype.institution = function () {
      return this.isIndirect() ? this._iban.substr(7, 4) : "";
    }, s.prototype.client = function () {
      return this.isIndirect() ? this._iban.substr(11) : "";
    }, s.prototype.address = function () {
      if (this.isDirect()) {
        var t = this._iban.substr(4),
            e = new r(t, 36);return o(e.toString(16), 20);
      }return "";
    }, s.prototype.toString = function () {
      return this._iban;
    }, e.exports = s;
  }, { "bignumber.js": "bignumber.js" }], 34: [function (t, e, n) {
    "use strict";
    var r = t("../utils/utils"),
        o = t("./errors"),
        i = function (t, e) {
      var n = this;this.responseCallbacks = {}, this.path = t, this.connection = e.connect({ path: this.path }), this.connection.on("error", function (t) {
        console.error("IPC Connection Error", t), n._timeout();
      }), this.connection.on("end", function () {
        n._timeout();
      }), this.connection.on("data", function (t) {
        n._parseResponse(t.toString()).forEach(function (t) {
          var e = null;r.isArray(t) ? t.forEach(function (t) {
            n.responseCallbacks[t.id] && (e = t.id);
          }) : e = t.id, n.responseCallbacks[e] && (n.responseCallbacks[e](null, t), delete n.responseCallbacks[e]);
        });
      });
    };i.prototype._parseResponse = function (t) {
      var e = this,
          n = [];return t.replace(/\}[\n\r]?\{/g, "}|--|{").replace(/\}\][\n\r]?\[\{/g, "}]|--|[{").replace(/\}[\n\r]?\[\{/g, "}|--|[{").replace(/\}\][\n\r]?\{/g, "}]|--|{").split("|--|").forEach(function (t) {
        e.lastChunk && (t = e.lastChunk + t);var r = null;try {
          r = JSON.parse(t);
        } catch (n) {
          return e.lastChunk = t, clearTimeout(e.lastChunkTimeout), void (e.lastChunkTimeout = setTimeout(function () {
            throw e._timeout(), o.InvalidResponse(t);
          }, 15e3));
        }clearTimeout(e.lastChunkTimeout), e.lastChunk = null, r && n.push(r);
      }), n;
    }, i.prototype._addResponseCallback = function (t, e) {
      var n = t.id || t[0].id,
          r = t.method || t[0].method;this.responseCallbacks[n] = e, this.responseCallbacks[n].method = r;
    }, i.prototype._timeout = function () {
      for (var t in this.responseCallbacks) this.responseCallbacks.hasOwnProperty(t) && (this.responseCallbacks[t](o.InvalidConnection("on IPC")), delete this.responseCallbacks[t]);
    }, i.prototype.isConnected = function () {
      var t = this;return t.connection.writable || t.connection.connect({ path: t.path }), !!this.connection.writable;
    }, i.prototype.send = function (t) {
      if (this.connection.writeSync) {
        var e;this.connection.writable || this.connection.connect({ path: this.path });var n = this.connection.writeSync(JSON.stringify(t));try {
          e = JSON.parse(n);
        } catch (t) {
          throw o.InvalidResponse(n);
        }return e;
      }throw new Error('You tried to send "' + t.method + '" synchronously. Synchronous requests are not supported by the IPC provider.');
    }, i.prototype.sendAsync = function (t, e) {
      this.connection.writable || this.connection.connect({ path: this.path }), this.connection.write(JSON.stringify(t)), this._addResponseCallback(t, e);
    }, e.exports = i;
  }, { "../utils/utils": 20, "./errors": 26 }], 35: [function (t, e, n) {
    var r = { messageId: 0 };r.toPayload = function (t, e) {
      return t || console.error("jsonrpc method should be specified!"), r.messageId++, { jsonrpc: "2.0", id: r.messageId, method: t, params: e || [] };
    }, r.isValidResponse = function (t) {
      function e(t) {
        return !!t && !t.error && "2.0" === t.jsonrpc && "number" == typeof t.id && void 0 !== t.result;
      }return Array.isArray(t) ? t.every(e) : e(t);
    }, r.toBatchPayload = function (t) {
      return t.map(function (t) {
        return r.toPayload(t.method, t.params);
      });
    }, e.exports = r;
  }, {}], 36: [function (t, e, n) {
    var r = t("../utils/utils"),
        o = t("./errors"),
        i = function (t) {
      this.name = t.name, this.call = t.call, this.params = t.params || 0, this.inputFormatter = t.inputFormatter, this.outputFormatter = t.outputFormatter, this.requestManager = null;
    };i.prototype.setRequestManager = function (t) {
      this.requestManager = t;
    }, i.prototype.getCall = function (t) {
      return r.isFunction(this.call) ? this.call(t) : this.call;
    }, i.prototype.extractCallback = function (t) {
      if (r.isFunction(t[t.length - 1])) return t.pop();
    }, i.prototype.validateArgs = function (t) {
      if (t.length !== this.params) throw o.InvalidNumberOfRPCParams();
    }, i.prototype.formatInput = function (t) {
      return this.inputFormatter ? this.inputFormatter.map(function (e, n) {
        return e ? e(t[n]) : t[n];
      }) : t;
    }, i.prototype.formatOutput = function (t) {
      return this.outputFormatter && t ? this.outputFormatter(t) : t;
    }, i.prototype.toPayload = function (t) {
      var e = this.getCall(t),
          n = this.extractCallback(t),
          r = this.formatInput(t);return this.validateArgs(r), { method: e, params: r, callback: n };
    }, i.prototype.attachToObject = function (t) {
      var e = this.buildCall();e.call = this.call;var n = this.name.split(".");n.length > 1 ? (t[n[0]] = t[n[0]] || {}, t[n[0]][n[1]] = e) : t[n[0]] = e;
    }, i.prototype.buildCall = function () {
      var t = this,
          e = function () {
        var e = t.toPayload(Array.prototype.slice.call(arguments));return e.callback ? t.requestManager.sendAsync(e, function (n, r) {
          e.callback(n, t.formatOutput(r));
        }) : t.formatOutput(t.requestManager.send(e));
      };return e.request = this.request.bind(this), e;
    }, i.prototype.request = function () {
      var t = this.toPayload(Array.prototype.slice.call(arguments));return t.format = this.formatOutput.bind(this), t;
    }, e.exports = i;
  }, { "../utils/utils": 20, "./errors": 26 }], 37: [function (t, e, n) {
    var r = t("../method"),
        o = function () {
      return [new r({ name: "putString", call: "db_putString", params: 3 }), new r({ name: "getString", call: "db_getString", params: 2 }), new r({ name: "putHex", call: "db_putHex", params: 3 }), new r({ name: "getHex", call: "db_getHex", params: 2 })];
    };e.exports = function (t) {
      this._requestManager = t._requestManager;var e = this;o().forEach(function (n) {
        n.attachToObject(e), n.setRequestManager(t._requestManager);
      });
    };
  }, { "../method": 36 }], 38: [function (t, e, n) {
    "use strict";
    function r(t) {
      this._requestManager = t._requestManager;var e = this;w().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      }), x().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      }), this.iban = d, this.sendIBANTransaction = m.bind(null, this);
    }var o = t("../formatters"),
        i = t("../../utils/utils"),
        a = t("../method"),
        s = t("../property"),
        c = t("../../utils/config"),
        u = t("../contract"),
        f = t("./watches"),
        l = t("../filter"),
        p = t("../syncing"),
        h = t("../namereg"),
        d = t("../iban"),
        m = t("../transfer"),
        y = function (t) {
      return i.isString(t[0]) && 0 === t[0].indexOf("0x") ? "eth_getBlockByHash" : "eth_getBlockByNumber";
    },
        g = function (t) {
      return i.isString(t[0]) && 0 === t[0].indexOf("0x") ? "eth_getTransactionByBlockHashAndIndex" : "eth_getTransactionByBlockNumberAndIndex";
    },
        v = function (t) {
      return i.isString(t[0]) && 0 === t[0].indexOf("0x") ? "eth_getUncleByBlockHashAndIndex" : "eth_getUncleByBlockNumberAndIndex";
    },
        b = function (t) {
      return i.isString(t[0]) && 0 === t[0].indexOf("0x") ? "eth_getBlockTransactionCountByHash" : "eth_getBlockTransactionCountByNumber";
    },
        _ = function (t) {
      return i.isString(t[0]) && 0 === t[0].indexOf("0x") ? "eth_getUncleCountByBlockHash" : "eth_getUncleCountByBlockNumber";
    };Object.defineProperty(r.prototype, "defaultBlock", { get: function () {
        return c.defaultBlock;
      }, set: function (t) {
        return c.defaultBlock = t, t;
      } }), Object.defineProperty(r.prototype, "defaultAccount", { get: function () {
        return c.defaultAccount;
      }, set: function (t) {
        return c.defaultAccount = t, t;
      } });var w = function () {
      var t = new a({ name: "getBalance", call: "eth_getBalance", params: 2, inputFormatter: [o.inputAddressFormatter, o.inputDefaultBlockNumberFormatter], outputFormatter: o.outputBigNumberFormatter }),
          e = new a({ name: "getStorageAt", call: "eth_getStorageAt", params: 3, inputFormatter: [null, i.toHex, o.inputDefaultBlockNumberFormatter] }),
          n = new a({ name: "getCode", call: "eth_getCode", params: 2, inputFormatter: [o.inputAddressFormatter, o.inputDefaultBlockNumberFormatter] }),
          r = new a({ name: "getBlock", call: y, params: 2, inputFormatter: [o.inputBlockNumberFormatter, function (t) {
          return !!t;
        }], outputFormatter: o.outputBlockFormatter }),
          s = new a({ name: "getUncle", call: v, params: 2, inputFormatter: [o.inputBlockNumberFormatter, i.toHex], outputFormatter: o.outputBlockFormatter }),
          c = new a({ name: "getCompilers", call: "eth_getCompilers", params: 0 }),
          u = new a({ name: "getBlockTransactionCount", call: b, params: 1, inputFormatter: [o.inputBlockNumberFormatter], outputFormatter: i.toDecimal }),
          f = new a({ name: "getBlockUncleCount", call: _, params: 1, inputFormatter: [o.inputBlockNumberFormatter], outputFormatter: i.toDecimal }),
          l = new a({ name: "getTransaction", call: "eth_getTransactionByHash", params: 1, outputFormatter: o.outputTransactionFormatter }),
          p = new a({ name: "getTransactionFromBlock", call: g, params: 2, inputFormatter: [o.inputBlockNumberFormatter, i.toHex], outputFormatter: o.outputTransactionFormatter }),
          h = new a({ name: "getTransactionReceipt", call: "eth_getTransactionReceipt", params: 1, outputFormatter: o.outputTransactionReceiptFormatter }),
          d = new a({ name: "getTransactionCount", call: "eth_getTransactionCount", params: 2, inputFormatter: [null, o.inputDefaultBlockNumberFormatter], outputFormatter: i.toDecimal }),
          m = new a({ name: "sendRawTransaction", call: "eth_sendRawTransaction", params: 1, inputFormatter: [null] }),
          w = new a({ name: "sendTransaction", call: "eth_sendTransaction", params: 1, inputFormatter: [o.inputTransactionFormatter] }),
          x = new a({ name: "signTransaction", call: "eth_signTransaction", params: 1, inputFormatter: [o.inputTransactionFormatter] }),
          k = new a({ name: "sign", call: "eth_sign", params: 2, inputFormatter: [o.inputAddressFormatter, null] });return [t, e, n, r, s, c, u, f, l, p, h, d, new a({ name: "call", call: "eth_call", params: 2, inputFormatter: [o.inputCallFormatter, o.inputDefaultBlockNumberFormatter] }), new a({ name: "estimateGas", call: "eth_estimateGas", params: 1, inputFormatter: [o.inputCallFormatter], outputFormatter: i.toDecimal }), m, x, w, k, new a({ name: "compile.solidity", call: "eth_compileSolidity", params: 1 }), new a({ name: "compile.lll", call: "eth_compileLLL", params: 1 }), new a({ name: "compile.serpent", call: "eth_compileSerpent", params: 1 }), new a({ name: "submitWork", call: "eth_submitWork", params: 3 }), new a({ name: "getWork", call: "eth_getWork", params: 0 })];
    },
        x = function () {
      return [new s({ name: "coinbase", getter: "eth_coinbase" }), new s({ name: "mining", getter: "eth_mining" }), new s({ name: "hashrate", getter: "eth_hashrate", outputFormatter: i.toDecimal }), new s({ name: "syncing", getter: "eth_syncing", outputFormatter: o.outputSyncingFormatter }), new s({ name: "gasPrice", getter: "eth_gasPrice", outputFormatter: o.outputBigNumberFormatter }), new s({ name: "accounts", getter: "eth_accounts" }), new s({ name: "blockNumber", getter: "eth_blockNumber", outputFormatter: i.toDecimal }), new s({ name: "protocolVersion", getter: "eth_protocolVersion" })];
    };r.prototype.contract = function (t) {
      return new u(this, t);
    }, r.prototype.filter = function (t, e, n) {
      return new l(t, "eth", this._requestManager, f.eth(), o.outputLogFormatter, e, n);
    }, r.prototype.namereg = function () {
      return this.contract(h.global.abi).at(h.global.address);
    }, r.prototype.icapNamereg = function () {
      return this.contract(h.icap.abi).at(h.icap.address);
    }, r.prototype.isSyncing = function (t) {
      return new p(this._requestManager, t);
    }, e.exports = r;
  }, { "../../utils/config": 18, "../../utils/utils": 20, "../contract": 25, "../filter": 29, "../formatters": 30, "../iban": 33, "../method": 36, "../namereg": 44, "../property": 45, "../syncing": 48, "../transfer": 49, "./watches": 43 }], 39: [function (t, e, n) {
    var r = t("../../utils/utils"),
        o = t("../property"),
        i = function () {
      return [new o({ name: "listening", getter: "net_listening" }), new o({ name: "peerCount", getter: "net_peerCount", outputFormatter: r.toDecimal })];
    };e.exports = function (t) {
      this._requestManager = t._requestManager;var e = this;i().forEach(function (n) {
        n.attachToObject(e), n.setRequestManager(t._requestManager);
      });
    };
  }, { "../../utils/utils": 20, "../property": 45 }], 40: [function (t, e, n) {
    "use strict";
    var r = t("../method"),
        o = t("../property"),
        i = t("../formatters"),
        a = function () {
      var t = new r({ name: "newAccount", call: "personal_newAccount", params: 1, inputFormatter: [null] }),
          e = new r({ name: "importRawKey", call: "personal_importRawKey", params: 2 }),
          n = new r({ name: "sign", call: "personal_sign", params: 3, inputFormatter: [null, i.inputAddressFormatter, null] }),
          o = new r({ name: "ecRecover", call: "personal_ecRecover", params: 2 });return [t, e, new r({ name: "unlockAccount", call: "personal_unlockAccount", params: 3, inputFormatter: [i.inputAddressFormatter, null, null] }), o, n, new r({ name: "sendTransaction", call: "personal_sendTransaction", params: 2, inputFormatter: [i.inputTransactionFormatter, null] }), new r({ name: "lockAccount", call: "personal_lockAccount", params: 1, inputFormatter: [i.inputAddressFormatter] })];
    },
        s = function () {
      return [new o({ name: "listAccounts", getter: "personal_listAccounts" })];
    };e.exports = function (t) {
      this._requestManager = t._requestManager;var e = this;a().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      }), s().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      });
    };
  }, { "../formatters": 30, "../method": 36, "../property": 45 }], 41: [function (t, e, n) {
    var r = t("../method"),
        o = t("../filter"),
        i = t("./watches"),
        a = function (t) {
      this._requestManager = t._requestManager;var e = this;s().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      });
    };a.prototype.newMessageFilter = function (t, e, n) {
      return new o(t, "shh", this._requestManager, i.shh(), null, e, n);
    };var s = function () {
      return [new r({ name: "version", call: "shh_version", params: 0 }), new r({ name: "info", call: "shh_info", params: 0 }), new r({ name: "setMaxMessageSize", call: "shh_setMaxMessageSize", params: 1 }), new r({ name: "setMinPoW", call: "shh_setMinPoW", params: 1 }), new r({ name: "markTrustedPeer", call: "shh_markTrustedPeer", params: 1 }), new r({ name: "newKeyPair", call: "shh_newKeyPair", params: 0 }), new r({ name: "addPrivateKey", call: "shh_addPrivateKey", params: 1 }), new r({ name: "deleteKeyPair", call: "shh_deleteKeyPair", params: 1 }), new r({ name: "hasKeyPair", call: "shh_hasKeyPair", params: 1 }), new r({ name: "getPublicKey", call: "shh_getPublicKey", params: 1 }), new r({ name: "getPrivateKey", call: "shh_getPrivateKey", params: 1 }), new r({ name: "newSymKey", call: "shh_newSymKey", params: 0 }), new r({ name: "addSymKey", call: "shh_addSymKey", params: 1 }), new r({ name: "generateSymKeyFromPassword", call: "shh_generateSymKeyFromPassword", params: 1 }), new r({ name: "hasSymKey", call: "shh_hasSymKey", params: 1 }), new r({ name: "getSymKey", call: "shh_getSymKey", params: 1 }), new r({ name: "deleteSymKey", call: "shh_deleteSymKey", params: 1 }), new r({ name: "post", call: "shh_post", params: 1, inputFormatter: [null] })];
    };e.exports = a;
  }, { "../filter": 29, "../method": 36, "./watches": 43 }], 42: [function (t, e, n) {
    "use strict";
    var r = t("../method"),
        o = t("../property"),
        i = function () {
      return [new r({ name: "blockNetworkRead", call: "bzz_blockNetworkRead", params: 1, inputFormatter: [null] }), new r({ name: "syncEnabled", call: "bzz_syncEnabled", params: 1, inputFormatter: [null] }), new r({ name: "swapEnabled", call: "bzz_swapEnabled", params: 1, inputFormatter: [null] }), new r({ name: "download", call: "bzz_download", params: 2, inputFormatter: [null, null] }), new r({ name: "upload", call: "bzz_upload", params: 2, inputFormatter: [null, null] }), new r({ name: "retrieve", call: "bzz_retrieve", params: 1, inputFormatter: [null] }), new r({ name: "store", call: "bzz_store", params: 2, inputFormatter: [null, null] }), new r({ name: "get", call: "bzz_get", params: 1, inputFormatter: [null] }), new r({ name: "put", call: "bzz_put", params: 2, inputFormatter: [null, null] }), new r({ name: "modify", call: "bzz_modify", params: 4, inputFormatter: [null, null, null, null] })];
    },
        a = function () {
      return [new o({ name: "hive", getter: "bzz_hive" }), new o({ name: "info", getter: "bzz_info" })];
    };e.exports = function (t) {
      this._requestManager = t._requestManager;var e = this;i().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      }), a().forEach(function (t) {
        t.attachToObject(e), t.setRequestManager(e._requestManager);
      });
    };
  }, { "../method": 36, "../property": 45 }], 43: [function (t, e, n) {
    var r = t("../method");e.exports = { eth: function () {
        return [new r({ name: "newFilter", call: function (t) {
            switch (t[0]) {case "latest":
                return t.shift(), this.params = 0, "eth_newBlockFilter";case "pending":
                return t.shift(), this.params = 0, "eth_newPendingTransactionFilter";default:
                return "eth_newFilter";}
          }, params: 1 }), new r({ name: "uninstallFilter", call: "eth_uninstallFilter", params: 1 }), new r({ name: "getLogs", call: "eth_getFilterLogs", params: 1 }), new r({ name: "poll", call: "eth_getFilterChanges", params: 1 })];
      }, shh: function () {
        return [new r({ name: "newFilter", call: "shh_newMessageFilter", params: 1 }), new r({ name: "uninstallFilter", call: "shh_deleteMessageFilter", params: 1 }), new r({ name: "getLogs", call: "shh_getFilterMessages", params: 1 }), new r({ name: "poll", call: "shh_getFilterMessages", params: 1 })];
      } };
  }, { "../method": 36 }], 44: [function (t, e, n) {
    var r = t("../contracts/GlobalRegistrar.json"),
        o = t("../contracts/ICAPRegistrar.json");e.exports = { global: { abi: r, address: "0xc6d9d2cd449a754c494264e1809c50e34d64562b" }, icap: { abi: o, address: "0xa1a111bc074c9cfa781f0c38e63bd51c91b8af00" } };
  }, { "../contracts/GlobalRegistrar.json": 1, "../contracts/ICAPRegistrar.json": 2 }], 45: [function (t, e, n) {
    var r = t("../utils/utils"),
        o = function (t) {
      this.name = t.name, this.getter = t.getter, this.setter = t.setter, this.outputFormatter = t.outputFormatter, this.inputFormatter = t.inputFormatter, this.requestManager = null;
    };o.prototype.setRequestManager = function (t) {
      this.requestManager = t;
    }, o.prototype.formatInput = function (t) {
      return this.inputFormatter ? this.inputFormatter(t) : t;
    }, o.prototype.formatOutput = function (t) {
      return this.outputFormatter && null !== t && void 0 !== t ? this.outputFormatter(t) : t;
    }, o.prototype.extractCallback = function (t) {
      if (r.isFunction(t[t.length - 1])) return t.pop();
    }, o.prototype.attachToObject = function (t) {
      var e = { get: this.buildGet(), enumerable: !0 },
          n = this.name.split("."),
          r = n[0];n.length > 1 && (t[n[0]] = t[n[0]] || {}, t = t[n[0]], r = n[1]), Object.defineProperty(t, r, e), t[i(r)] = this.buildAsyncGet();
    };var i = function (t) {
      return "get" + t.charAt(0).toUpperCase() + t.slice(1);
    };o.prototype.buildGet = function () {
      var t = this;return function () {
        return t.formatOutput(t.requestManager.send({ method: t.getter }));
      };
    }, o.prototype.buildAsyncGet = function () {
      var t = this,
          e = function (e) {
        t.requestManager.sendAsync({ method: t.getter }, function (n, r) {
          e(n, t.formatOutput(r));
        });
      };return e.request = this.request.bind(this), e;
    }, o.prototype.request = function () {
      var t = { method: this.getter, params: [], callback: this.extractCallback(Array.prototype.slice.call(arguments)) };return t.format = this.formatOutput.bind(this), t;
    }, e.exports = o;
  }, { "../utils/utils": 20 }], 46: [function (t, e, n) {
    var r = t("./jsonrpc"),
        o = t("../utils/utils"),
        i = t("../utils/config"),
        a = t("./errors"),
        s = function (t) {
      this.provider = t, this.polls = {}, this.timeout = null;
    };s.prototype.send = function (t) {
      if (!this.provider) return console.error(a.InvalidProvider()), null;var e = r.toPayload(t.method, t.params),
          n = this.provider.send(e);if (!r.isValidResponse(n)) throw a.InvalidResponse(n);return n.result;
    }, s.prototype.sendAsync = function (t, e) {
      if (!this.provider) return e(a.InvalidProvider());var n = r.toPayload(t.method, t.params);this.provider.sendAsync(n, function (t, n) {
        return t ? e(t) : r.isValidResponse(n) ? void e(null, n.result) : e(a.InvalidResponse(n));
      });
    }, s.prototype.sendBatch = function (t, e) {
      if (!this.provider) return e(a.InvalidProvider());var n = r.toBatchPayload(t);this.provider.sendAsync(n, function (t, n) {
        return t ? e(t) : o.isArray(n) ? void e(t, n) : e(a.InvalidResponse(n));
      });
    }, s.prototype.setProvider = function (t) {
      this.provider = t;
    }, s.prototype.startPolling = function (t, e, n, r) {
      this.polls[e] = { data: t, id: e, callback: n, uninstall: r }, this.timeout || this.poll();
    }, s.prototype.stopPolling = function (t) {
      delete this.polls[t], 0 === Object.keys(this.polls).length && this.timeout && (clearTimeout(this.timeout), this.timeout = null);
    }, s.prototype.reset = function (t) {
      for (var e in this.polls) t && -1 !== e.indexOf("syncPoll_") || (this.polls[e].uninstall(), delete this.polls[e]);0 === Object.keys(this.polls).length && this.timeout && (clearTimeout(this.timeout), this.timeout = null);
    }, s.prototype.poll = function () {
      if (this.timeout = setTimeout(this.poll.bind(this), i.ETH_POLLING_TIMEOUT), 0 !== Object.keys(this.polls).length) if (this.provider) {
        var t = [],
            e = [];for (var n in this.polls) t.push(this.polls[n].data), e.push(n);if (0 !== t.length) {
          var s = r.toBatchPayload(t),
              c = {};s.forEach(function (t, n) {
            c[t.id] = e[n];
          });var u = this;this.provider.sendAsync(s, function (t, e) {
            if (!t) {
              if (!o.isArray(e)) throw a.InvalidResponse(e);e.map(function (t) {
                var e = c[t.id];return !!u.polls[e] && (t.callback = u.polls[e].callback, t);
              }).filter(function (t) {
                return !!t;
              }).filter(function (t) {
                var e = r.isValidResponse(t);return e || t.callback(a.InvalidResponse(t)), e;
              }).forEach(function (t) {
                t.callback(null, t.result);
              });
            }
          });
        }
      } else console.error(a.InvalidProvider());
    }, e.exports = s;
  }, { "../utils/config": 18, "../utils/utils": 20, "./errors": 26, "./jsonrpc": 35 }], 47: [function (t, e, n) {
    e.exports = function () {
      this.defaultBlock = "latest", this.defaultAccount = void 0;
    };
  }, {}], 48: [function (t, e, n) {
    var r = t("./formatters"),
        o = t("../utils/utils"),
        i = 1,
        a = function (t) {
      t.requestManager.startPolling({ method: "eth_syncing", params: [] }, t.pollId, function (e, n) {
        if (e) return t.callbacks.forEach(function (t) {
          t(e);
        });o.isObject(n) && n.startingBlock && (n = r.outputSyncingFormatter(n)), t.callbacks.forEach(function (e) {
          t.lastSyncState !== n && (!t.lastSyncState && o.isObject(n) && e(null, !0), setTimeout(function () {
            e(null, n);
          }, 0), t.lastSyncState = n);
        });
      }, t.stopWatching.bind(t));
    },
        s = function (t, e) {
      return this.requestManager = t, this.pollId = "syncPoll_" + i++, this.callbacks = [], this.addCallback(e), this.lastSyncState = !1, a(this), this;
    };s.prototype.addCallback = function (t) {
      return t && this.callbacks.push(t), this;
    }, s.prototype.stopWatching = function () {
      this.requestManager.stopPolling(this.pollId), this.callbacks = [];
    }, e.exports = s;
  }, { "../utils/utils": 20, "./formatters": 30 }], 49: [function (t, e, n) {
    var r = t("./iban"),
        o = t("../contracts/SmartExchange.json"),
        i = function (t, e, n, r, o) {
      return t.sendTransaction({ address: n, from: e, value: r }, o);
    },
        a = function (t, e, n, r, i, a) {
      var s = o;return t.contract(s).at(n).deposit(i, { from: e, value: r }, a);
    };e.exports = function (t, e, n, o, s) {
      var c = new r(n);if (!c.isValid()) throw new Error("invalid iban address");if (c.isDirect()) return i(t, e, c.address(), o, s);if (!s) {
        var u = t.icapNamereg().addr(c.institution());return a(t, e, u, o, c.client());
      }t.icapNamereg().addr(c.institution(), function (n, r) {
        return a(t, e, r, o, c.client(), s);
      });
    };
  }, { "../contracts/SmartExchange.json": 3, "./iban": 33 }], 50: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./enc-base64"), t("./md5"), t("./evpkdf"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./enc-base64", "./md5", "./evpkdf", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.lib.BlockCipher,
            r = e.algo,
            o = [],
            i = [],
            a = [],
            s = [],
            c = [],
            u = [],
            f = [],
            l = [],
            p = [],
            h = [];!function () {
          for (var t = [], e = 0; e < 256; e++) t[e] = e < 128 ? e << 1 : e << 1 ^ 283;for (var n = 0, r = 0, e = 0; e < 256; e++) {
            var d = r ^ r << 1 ^ r << 2 ^ r << 3 ^ r << 4;d = d >>> 8 ^ 255 & d ^ 99, o[n] = d, i[d] = n;var m = t[n],
                y = t[m],
                g = t[y],
                v = 257 * t[d] ^ 16843008 * d;a[n] = v << 24 | v >>> 8, s[n] = v << 16 | v >>> 16, c[n] = v << 8 | v >>> 24, u[n] = v;v = 16843009 * g ^ 65537 * y ^ 257 * m ^ 16843008 * n;f[d] = v << 24 | v >>> 8, l[d] = v << 16 | v >>> 16, p[d] = v << 8 | v >>> 24, h[d] = v, n ? (n = m ^ t[t[t[g ^ m]]], r ^= t[t[r]]) : n = r = 1;
          }
        }();var d = [0, 1, 2, 4, 8, 16, 32, 64, 128, 27, 54],
            m = r.AES = n.extend({ _doReset: function () {
            if (!this._nRounds || this._keyPriorReset !== this._key) {
              for (var t = this._keyPriorReset = this._key, e = t.words, n = t.sigBytes / 4, r = 4 * ((this._nRounds = n + 6) + 1), i = this._keySchedule = [], a = 0; a < r; a++) if (a < n) i[a] = e[a];else {
                u = i[a - 1];a % n ? n > 6 && a % n == 4 && (u = o[u >>> 24] << 24 | o[u >>> 16 & 255] << 16 | o[u >>> 8 & 255] << 8 | o[255 & u]) : (u = o[(u = u << 8 | u >>> 24) >>> 24] << 24 | o[u >>> 16 & 255] << 16 | o[u >>> 8 & 255] << 8 | o[255 & u], u ^= d[a / n | 0] << 24), i[a] = i[a - n] ^ u;
              }for (var s = this._invKeySchedule = [], c = 0; c < r; c++) {
                a = r - c;if (c % 4) u = i[a];else var u = i[a - 4];s[c] = c < 4 || a <= 4 ? u : f[o[u >>> 24]] ^ l[o[u >>> 16 & 255]] ^ p[o[u >>> 8 & 255]] ^ h[o[255 & u]];
              }
            }
          }, encryptBlock: function (t, e) {
            this._doCryptBlock(t, e, this._keySchedule, a, s, c, u, o);
          }, decryptBlock: function (t, e) {
            n = t[e + 1];t[e + 1] = t[e + 3], t[e + 3] = n, this._doCryptBlock(t, e, this._invKeySchedule, f, l, p, h, i);var n = t[e + 1];t[e + 1] = t[e + 3], t[e + 3] = n;
          }, _doCryptBlock: function (t, e, n, r, o, i, a, s) {
            for (var c = this._nRounds, u = t[e] ^ n[0], f = t[e + 1] ^ n[1], l = t[e + 2] ^ n[2], p = t[e + 3] ^ n[3], h = 4, d = 1; d < c; d++) {
              var m = r[u >>> 24] ^ o[f >>> 16 & 255] ^ i[l >>> 8 & 255] ^ a[255 & p] ^ n[h++],
                  y = r[f >>> 24] ^ o[l >>> 16 & 255] ^ i[p >>> 8 & 255] ^ a[255 & u] ^ n[h++],
                  g = r[l >>> 24] ^ o[p >>> 16 & 255] ^ i[u >>> 8 & 255] ^ a[255 & f] ^ n[h++],
                  v = r[p >>> 24] ^ o[u >>> 16 & 255] ^ i[f >>> 8 & 255] ^ a[255 & l] ^ n[h++];u = m, f = y, l = g, p = v;
            }var m = (s[u >>> 24] << 24 | s[f >>> 16 & 255] << 16 | s[l >>> 8 & 255] << 8 | s[255 & p]) ^ n[h++],
                y = (s[f >>> 24] << 24 | s[l >>> 16 & 255] << 16 | s[p >>> 8 & 255] << 8 | s[255 & u]) ^ n[h++],
                g = (s[l >>> 24] << 24 | s[p >>> 16 & 255] << 16 | s[u >>> 8 & 255] << 8 | s[255 & f]) ^ n[h++],
                v = (s[p >>> 24] << 24 | s[u >>> 16 & 255] << 16 | s[f >>> 8 & 255] << 8 | s[255 & l]) ^ n[h++];t[e] = m, t[e + 1] = y, t[e + 2] = g, t[e + 3] = v;
          }, keySize: 8 });e.AES = n._createHelper(m);
      }(), t.AES;
    });
  }, { "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./evpkdf": 55, "./md5": 60 }], 51: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      t.lib.Cipher || function (e) {
        var n = t,
            r = n.lib,
            o = r.Base,
            i = r.WordArray,
            a = r.BufferedBlockAlgorithm,
            s = n.enc,
            c = (s.Utf8, s.Base64),
            u = n.algo.EvpKDF,
            f = r.Cipher = a.extend({ cfg: o.extend(), createEncryptor: function (t, e) {
            return this.create(this._ENC_XFORM_MODE, t, e);
          }, createDecryptor: function (t, e) {
            return this.create(this._DEC_XFORM_MODE, t, e);
          }, init: function (t, e, n) {
            this.cfg = this.cfg.extend(n), this._xformMode = t, this._key = e, this.reset();
          }, reset: function () {
            a.reset.call(this), this._doReset();
          }, process: function (t) {
            return this._append(t), this._process();
          }, finalize: function (t) {
            return t && this._append(t), this._doFinalize();
          }, keySize: 4, ivSize: 4, _ENC_XFORM_MODE: 1, _DEC_XFORM_MODE: 2, _createHelper: function () {
            function t(t) {
              return "string" == typeof t ? b : g;
            }return function (e) {
              return { encrypt: function (n, r, o) {
                  return t(r).encrypt(e, n, r, o);
                }, decrypt: function (n, r, o) {
                  return t(r).decrypt(e, n, r, o);
                } };
            };
          }() }),
            l = (r.StreamCipher = f.extend({ _doFinalize: function () {
            return this._process(!0);
          }, blockSize: 1 }), n.mode = {}),
            p = r.BlockCipherMode = o.extend({ createEncryptor: function (t, e) {
            return this.Encryptor.create(t, e);
          }, createDecryptor: function (t, e) {
            return this.Decryptor.create(t, e);
          }, init: function (t, e) {
            this._cipher = t, this._iv = e;
          } }),
            h = l.CBC = function () {
          function t(t, n, r) {
            var o = this._iv;if (o) {
              i = o;this._iv = e;
            } else var i = this._prevBlock;for (var a = 0; a < r; a++) t[n + a] ^= i[a];
          }var n = p.extend();return n.Encryptor = n.extend({ processBlock: function (e, n) {
              var r = this._cipher,
                  o = r.blockSize;t.call(this, e, n, o), r.encryptBlock(e, n), this._prevBlock = e.slice(n, n + o);
            } }), n.Decryptor = n.extend({ processBlock: function (e, n) {
              var r = this._cipher,
                  o = r.blockSize,
                  i = e.slice(n, n + o);r.decryptBlock(e, n), t.call(this, e, n, o), this._prevBlock = i;
            } }), n;
        }(),
            d = (n.pad = {}).Pkcs7 = { pad: function (t, e) {
            for (var n = 4 * e, r = n - t.sigBytes % n, o = r << 24 | r << 16 | r << 8 | r, a = [], s = 0; s < r; s += 4) a.push(o);var c = i.create(a, r);t.concat(c);
          }, unpad: function (t) {
            var e = 255 & t.words[t.sigBytes - 1 >>> 2];t.sigBytes -= e;
          } },
            m = (r.BlockCipher = f.extend({ cfg: f.cfg.extend({ mode: h, padding: d }), reset: function () {
            f.reset.call(this);var t = this.cfg,
                e = t.iv,
                n = t.mode;if (this._xformMode == this._ENC_XFORM_MODE) r = n.createEncryptor;else {
              var r = n.createDecryptor;this._minBufferSize = 1;
            }this._mode = r.call(n, this, e && e.words);
          }, _doProcessBlock: function (t, e) {
            this._mode.processBlock(t, e);
          }, _doFinalize: function () {
            var t = this.cfg.padding;if (this._xformMode == this._ENC_XFORM_MODE) {
              t.pad(this._data, this.blockSize);e = this._process(!0);
            } else {
              var e = this._process(!0);t.unpad(e);
            }return e;
          }, blockSize: 4 }), r.CipherParams = o.extend({ init: function (t) {
            this.mixIn(t);
          }, toString: function (t) {
            return (t || this.formatter).stringify(this);
          } })),
            y = (n.format = {}).OpenSSL = { stringify: function (t) {
            var e = t.ciphertext,
                n = t.salt;if (n) r = i.create([1398893684, 1701076831]).concat(n).concat(e);else var r = e;return r.toString(c);
          }, parse: function (t) {
            var e = c.parse(t),
                n = e.words;if (1398893684 == n[0] && 1701076831 == n[1]) {
              var r = i.create(n.slice(2, 4));n.splice(0, 4), e.sigBytes -= 16;
            }return m.create({ ciphertext: e, salt: r });
          } },
            g = r.SerializableCipher = o.extend({ cfg: o.extend({ format: y }), encrypt: function (t, e, n, r) {
            r = this.cfg.extend(r);var o = t.createEncryptor(n, r),
                i = o.finalize(e),
                a = o.cfg;return m.create({ ciphertext: i, key: n, iv: a.iv, algorithm: t, mode: a.mode, padding: a.padding, blockSize: t.blockSize, formatter: r.format });
          }, decrypt: function (t, e, n, r) {
            return r = this.cfg.extend(r), e = this._parse(e, r.format), t.createDecryptor(n, r).finalize(e.ciphertext);
          }, _parse: function (t, e) {
            return "string" == typeof t ? e.parse(t, this) : t;
          } }),
            v = (n.kdf = {}).OpenSSL = { execute: function (t, e, n, r) {
            r || (r = i.random(8));var o = u.create({ keySize: e + n }).compute(t, r),
                a = i.create(o.words.slice(e), 4 * n);return o.sigBytes = 4 * e, m.create({ key: o, iv: a, salt: r });
          } },
            b = r.PasswordBasedCipher = g.extend({ cfg: g.cfg.extend({ kdf: v }), encrypt: function (t, e, n, r) {
            var o = (r = this.cfg.extend(r)).kdf.execute(n, t.keySize, t.ivSize);r.iv = o.iv;var i = g.encrypt.call(this, t, e, o.key, r);return i.mixIn(o), i;
          }, decrypt: function (t, e, n, r) {
            r = this.cfg.extend(r), e = this._parse(e, r.format);var o = r.kdf.execute(n, t.keySize, t.ivSize, e.salt);return r.iv = o.iv, g.decrypt.call(this, t, e, o.key, r);
          } });
      }();
    });
  }, { "./core": 52 }], 52: [function (t, e, n) {
    !function (t, r) {
      "object" == typeof n ? e.exports = n = r() : "function" == typeof define && define.amd ? define([], r) : t.CryptoJS = r();
    }(this, function () {
      var t = t || function (t, e) {
        var n = Object.create || function () {
          function t() {}return function (e) {
            var n;return t.prototype = e, n = new t(), t.prototype = null, n;
          };
        }(),
            r = {},
            o = r.lib = {},
            i = o.Base = { extend: function (t) {
            var e = n(this);return t && e.mixIn(t), e.hasOwnProperty("init") && this.init !== e.init || (e.init = function () {
              e.$super.init.apply(this, arguments);
            }), e.init.prototype = e, e.$super = this, e;
          }, create: function () {
            var t = this.extend();return t.init.apply(t, arguments), t;
          }, init: function () {}, mixIn: function (t) {
            for (var e in t) t.hasOwnProperty(e) && (this[e] = t[e]);t.hasOwnProperty("toString") && (this.toString = t.toString);
          }, clone: function () {
            return this.init.prototype.extend(this);
          } },
            a = o.WordArray = i.extend({ init: function (t, e) {
            t = this.words = t || [], this.sigBytes = void 0 != e ? e : 4 * t.length;
          }, toString: function (t) {
            return (t || c).stringify(this);
          }, concat: function (t) {
            var e = this.words,
                n = t.words,
                r = this.sigBytes,
                o = t.sigBytes;if (this.clamp(), r % 4) for (a = 0; a < o; a++) {
              var i = n[a >>> 2] >>> 24 - a % 4 * 8 & 255;e[r + a >>> 2] |= i << 24 - (r + a) % 4 * 8;
            } else for (var a = 0; a < o; a += 4) e[r + a >>> 2] = n[a >>> 2];return this.sigBytes += o, this;
          }, clamp: function () {
            var e = this.words,
                n = this.sigBytes;e[n >>> 2] &= 4294967295 << 32 - n % 4 * 8, e.length = t.ceil(n / 4);
          }, clone: function () {
            var t = i.clone.call(this);return t.words = this.words.slice(0), t;
          }, random: function (e) {
            for (var n, r = [], o = 0; o < e; o += 4) {
              var i = function (e) {
                var e = e,
                    n = 987654321,
                    r = 4294967295;return function () {
                  var o = ((n = 36969 * (65535 & n) + (n >> 16) & r) << 16) + (e = 18e3 * (65535 & e) + (e >> 16) & r) & r;return o /= 4294967296, (o += .5) * (t.random() > .5 ? 1 : -1);
                };
              }(4294967296 * (n || t.random()));n = 987654071 * i(), r.push(4294967296 * i() | 0);
            }return new a.init(r, e);
          } }),
            s = r.enc = {},
            c = s.Hex = { stringify: function (t) {
            for (var e = t.words, n = t.sigBytes, r = [], o = 0; o < n; o++) {
              var i = e[o >>> 2] >>> 24 - o % 4 * 8 & 255;r.push((i >>> 4).toString(16)), r.push((15 & i).toString(16));
            }return r.join("");
          }, parse: function (t) {
            for (var e = t.length, n = [], r = 0; r < e; r += 2) n[r >>> 3] |= parseInt(t.substr(r, 2), 16) << 24 - r % 8 * 4;return new a.init(n, e / 2);
          } },
            u = s.Latin1 = { stringify: function (t) {
            for (var e = t.words, n = t.sigBytes, r = [], o = 0; o < n; o++) {
              var i = e[o >>> 2] >>> 24 - o % 4 * 8 & 255;r.push(String.fromCharCode(i));
            }return r.join("");
          }, parse: function (t) {
            for (var e = t.length, n = [], r = 0; r < e; r++) n[r >>> 2] |= (255 & t.charCodeAt(r)) << 24 - r % 4 * 8;return new a.init(n, e);
          } },
            f = s.Utf8 = { stringify: function (t) {
            try {
              return decodeURIComponent(escape(u.stringify(t)));
            } catch (t) {
              throw new Error("Malformed UTF-8 data");
            }
          }, parse: function (t) {
            return u.parse(unescape(encodeURIComponent(t)));
          } },
            l = o.BufferedBlockAlgorithm = i.extend({ reset: function () {
            this._data = new a.init(), this._nDataBytes = 0;
          }, _append: function (t) {
            "string" == typeof t && (t = f.parse(t)), this._data.concat(t), this._nDataBytes += t.sigBytes;
          }, _process: function (e) {
            var n = this._data,
                r = n.words,
                o = n.sigBytes,
                i = this.blockSize,
                s = o / (4 * i),
                c = (s = e ? t.ceil(s) : t.max((0 | s) - this._minBufferSize, 0)) * i,
                u = t.min(4 * c, o);if (c) {
              for (var f = 0; f < c; f += i) this._doProcessBlock(r, f);var l = r.splice(0, c);n.sigBytes -= u;
            }return new a.init(l, u);
          }, clone: function () {
            var t = i.clone.call(this);return t._data = this._data.clone(), t;
          }, _minBufferSize: 0 }),
            p = (o.Hasher = l.extend({ cfg: i.extend(), init: function (t) {
            this.cfg = this.cfg.extend(t), this.reset();
          }, reset: function () {
            l.reset.call(this), this._doReset();
          }, update: function (t) {
            return this._append(t), this._process(), this;
          }, finalize: function (t) {
            return t && this._append(t), this._doFinalize();
          }, blockSize: 16, _createHelper: function (t) {
            return function (e, n) {
              return new t.init(n).finalize(e);
            };
          }, _createHmacHelper: function (t) {
            return function (e, n) {
              return new p.HMAC.init(t, n).finalize(e);
            };
          } }), r.algo = {});return r;
      }(Math);return t;
    });
  }, {}], 53: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e(t, e, n) {
          for (var o = [], i = 0, a = 0; a < e; a++) if (a % 4) {
            var s = n[t.charCodeAt(a - 1)] << a % 4 * 2,
                c = n[t.charCodeAt(a)] >>> 6 - a % 4 * 2;o[i >>> 2] |= (s | c) << 24 - i % 4 * 8, i++;
          }return r.create(o, i);
        }var n = t,
            r = n.lib.WordArray;n.enc.Base64 = { stringify: function (t) {
            var e = t.words,
                n = t.sigBytes,
                r = this._map;t.clamp();for (var o = [], i = 0; i < n; i += 3) for (var a = (e[i >>> 2] >>> 24 - i % 4 * 8 & 255) << 16 | (e[i + 1 >>> 2] >>> 24 - (i + 1) % 4 * 8 & 255) << 8 | e[i + 2 >>> 2] >>> 24 - (i + 2) % 4 * 8 & 255, s = 0; s < 4 && i + .75 * s < n; s++) o.push(r.charAt(a >>> 6 * (3 - s) & 63));var c = r.charAt(64);if (c) for (; o.length % 4;) o.push(c);return o.join("");
          }, parse: function (t) {
            var n = t.length,
                r = this._map,
                o = this._reverseMap;if (!o) {
              o = this._reverseMap = [];for (var i = 0; i < r.length; i++) o[r.charCodeAt(i)] = i;
            }var a = r.charAt(64);if (a) {
              var s = t.indexOf(a);-1 !== s && (n = s);
            }return e(t, n, o);
          }, _map: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=" };
      }(), t.enc.Base64;
    });
  }, { "./core": 52 }], 54: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e(t) {
          return t << 8 & 4278255360 | t >>> 8 & 16711935;
        }var n = t,
            r = n.lib.WordArray,
            o = n.enc;o.Utf16 = o.Utf16BE = { stringify: function (t) {
            for (var e = t.words, n = t.sigBytes, r = [], o = 0; o < n; o += 2) {
              var i = e[o >>> 2] >>> 16 - o % 4 * 8 & 65535;r.push(String.fromCharCode(i));
            }return r.join("");
          }, parse: function (t) {
            for (var e = t.length, n = [], o = 0; o < e; o++) n[o >>> 1] |= t.charCodeAt(o) << 16 - o % 2 * 16;return r.create(n, 2 * e);
          } };o.Utf16LE = { stringify: function (t) {
            for (var n = t.words, r = t.sigBytes, o = [], i = 0; i < r; i += 2) {
              var a = e(n[i >>> 2] >>> 16 - i % 4 * 8 & 65535);o.push(String.fromCharCode(a));
            }return o.join("");
          }, parse: function (t) {
            for (var n = t.length, o = [], i = 0; i < n; i++) o[i >>> 1] |= e(t.charCodeAt(i) << 16 - i % 2 * 16);return r.create(o, 2 * n);
          } };
      }(), t.enc.Utf16;
    });
  }, { "./core": 52 }], 55: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./sha1"), t("./hmac")) : "function" == typeof define && define.amd ? define(["./core", "./sha1", "./hmac"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.lib,
            r = n.Base,
            o = n.WordArray,
            i = e.algo,
            a = i.MD5,
            s = i.EvpKDF = r.extend({ cfg: r.extend({ keySize: 4, hasher: a, iterations: 1 }), init: function (t) {
            this.cfg = this.cfg.extend(t);
          }, compute: function (t, e) {
            for (var n = this.cfg, r = n.hasher.create(), i = o.create(), a = i.words, s = n.keySize, c = n.iterations; a.length < s;) {
              u && r.update(u);var u = r.update(t).finalize(e);r.reset();for (var f = 1; f < c; f++) u = r.finalize(u), r.reset();i.concat(u);
            }return i.sigBytes = 4 * s, i;
          } });e.EvpKDF = function (t, e, n) {
          return s.create(n).compute(t, e);
        };
      }(), t.EvpKDF;
    });
  }, { "./core": 52, "./hmac": 57, "./sha1": 76 }], 56: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        var n = t,
            r = n.lib.CipherParams,
            o = n.enc.Hex;n.format.Hex = { stringify: function (t) {
            return t.ciphertext.toString(o);
          }, parse: function (t) {
            var e = o.parse(t);return r.create({ ciphertext: e });
          } };
      }(), t.format.Hex;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 57: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      !function () {
        var e = t,
            n = e.lib.Base,
            r = e.enc.Utf8;e.algo.HMAC = n.extend({ init: function (t, e) {
            t = this._hasher = new t.init(), "string" == typeof e && (e = r.parse(e));var n = t.blockSize,
                o = 4 * n;e.sigBytes > o && (e = t.finalize(e)), e.clamp();for (var i = this._oKey = e.clone(), a = this._iKey = e.clone(), s = i.words, c = a.words, u = 0; u < n; u++) s[u] ^= 1549556828, c[u] ^= 909522486;i.sigBytes = a.sigBytes = o, this.reset();
          }, reset: function () {
            var t = this._hasher;t.reset(), t.update(this._iKey);
          }, update: function (t) {
            return this._hasher.update(t), this;
          }, finalize: function (t) {
            var e = this._hasher,
                n = e.finalize(t);return e.reset(), e.finalize(this._oKey.clone().concat(n));
          } });
      }();
    });
  }, { "./core": 52 }], 58: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./x64-core"), t("./lib-typedarrays"), t("./enc-utf16"), t("./enc-base64"), t("./md5"), t("./sha1"), t("./sha256"), t("./sha224"), t("./sha512"), t("./sha384"), t("./sha3"), t("./ripemd160"), t("./hmac"), t("./pbkdf2"), t("./evpkdf"), t("./cipher-core"), t("./mode-cfb"), t("./mode-ctr"), t("./mode-ctr-gladman"), t("./mode-ofb"), t("./mode-ecb"), t("./pad-ansix923"), t("./pad-iso10126"), t("./pad-iso97971"), t("./pad-zeropadding"), t("./pad-nopadding"), t("./format-hex"), t("./aes"), t("./tripledes"), t("./rc4"), t("./rabbit"), t("./rabbit-legacy")) : "function" == typeof define && define.amd ? define(["./core", "./x64-core", "./lib-typedarrays", "./enc-utf16", "./enc-base64", "./md5", "./sha1", "./sha256", "./sha224", "./sha512", "./sha384", "./sha3", "./ripemd160", "./hmac", "./pbkdf2", "./evpkdf", "./cipher-core", "./mode-cfb", "./mode-ctr", "./mode-ctr-gladman", "./mode-ofb", "./mode-ecb", "./pad-ansix923", "./pad-iso10126", "./pad-iso97971", "./pad-zeropadding", "./pad-nopadding", "./format-hex", "./aes", "./tripledes", "./rc4", "./rabbit", "./rabbit-legacy"], o) : r.CryptoJS = o(r.CryptoJS);
    }(this, function (t) {
      return t;
    });
  }, { "./aes": 50, "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./enc-utf16": 54, "./evpkdf": 55, "./format-hex": 56, "./hmac": 57, "./lib-typedarrays": 59, "./md5": 60, "./mode-cfb": 61, "./mode-ctr": 63, "./mode-ctr-gladman": 62, "./mode-ecb": 64, "./mode-ofb": 65, "./pad-ansix923": 66, "./pad-iso10126": 67, "./pad-iso97971": 68, "./pad-nopadding": 69, "./pad-zeropadding": 70, "./pbkdf2": 71, "./rabbit": 73, "./rabbit-legacy": 72, "./rc4": 74, "./ripemd160": 75, "./sha1": 76, "./sha224": 77, "./sha256": 78, "./sha3": 79, "./sha384": 80, "./sha512": 81, "./tripledes": 82, "./x64-core": 83 }], 59: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        if ("function" == typeof ArrayBuffer) {
          var e = t.lib.WordArray,
              n = e.init;(e.init = function (t) {
            if (t instanceof ArrayBuffer && (t = new Uint8Array(t)), (t instanceof Int8Array || "undefined" != typeof Uint8ClampedArray && t instanceof Uint8ClampedArray || t instanceof Int16Array || t instanceof Uint16Array || t instanceof Int32Array || t instanceof Uint32Array || t instanceof Float32Array || t instanceof Float64Array) && (t = new Uint8Array(t.buffer, t.byteOffset, t.byteLength)), t instanceof Uint8Array) {
              for (var e = t.byteLength, r = [], o = 0; o < e; o++) r[o >>> 2] |= t[o] << 24 - o % 4 * 8;n.call(this, r, e);
            } else n.apply(this, arguments);
          }).prototype = e;
        }
      }(), t.lib.WordArray;
    });
  }, { "./core": 52 }], 60: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        function n(t, e, n, r, o, i, a) {
          var s = t + (e & n | ~e & r) + o + a;return (s << i | s >>> 32 - i) + e;
        }function r(t, e, n, r, o, i, a) {
          var s = t + (e & r | n & ~r) + o + a;return (s << i | s >>> 32 - i) + e;
        }function o(t, e, n, r, o, i, a) {
          var s = t + (e ^ n ^ r) + o + a;return (s << i | s >>> 32 - i) + e;
        }function i(t, e, n, r, o, i, a) {
          var s = t + (n ^ (e | ~r)) + o + a;return (s << i | s >>> 32 - i) + e;
        }var a = t,
            s = a.lib,
            c = s.WordArray,
            u = s.Hasher,
            f = a.algo,
            l = [];!function () {
          for (var t = 0; t < 64; t++) l[t] = 4294967296 * e.abs(e.sin(t + 1)) | 0;
        }();var p = f.MD5 = u.extend({ _doReset: function () {
            this._hash = new c.init([1732584193, 4023233417, 2562383102, 271733878]);
          }, _doProcessBlock: function (t, e) {
            for (var a = 0; a < 16; a++) {
              var s = e + a,
                  c = t[s];t[s] = 16711935 & (c << 8 | c >>> 24) | 4278255360 & (c << 24 | c >>> 8);
            }var u = this._hash.words,
                f = t[e + 0],
                p = t[e + 1],
                h = t[e + 2],
                d = t[e + 3],
                m = t[e + 4],
                y = t[e + 5],
                g = t[e + 6],
                v = t[e + 7],
                b = t[e + 8],
                _ = t[e + 9],
                w = t[e + 10],
                x = t[e + 11],
                k = t[e + 12],
                B = t[e + 13],
                S = t[e + 14],
                C = t[e + 15],
                A = u[0],
                F = u[1],
                O = u[2],
                I = u[3];F = i(F = i(F = i(F = i(F = o(F = o(F = o(F = o(F = r(F = r(F = r(F = r(F = n(F = n(F = n(F = n(F, O = n(O, I = n(I, A = n(A, F, O, I, f, 7, l[0]), F, O, p, 12, l[1]), A, F, h, 17, l[2]), I, A, d, 22, l[3]), O = n(O, I = n(I, A = n(A, F, O, I, m, 7, l[4]), F, O, y, 12, l[5]), A, F, g, 17, l[6]), I, A, v, 22, l[7]), O = n(O, I = n(I, A = n(A, F, O, I, b, 7, l[8]), F, O, _, 12, l[9]), A, F, w, 17, l[10]), I, A, x, 22, l[11]), O = n(O, I = n(I, A = n(A, F, O, I, k, 7, l[12]), F, O, B, 12, l[13]), A, F, S, 17, l[14]), I, A, C, 22, l[15]), O = r(O, I = r(I, A = r(A, F, O, I, p, 5, l[16]), F, O, g, 9, l[17]), A, F, x, 14, l[18]), I, A, f, 20, l[19]), O = r(O, I = r(I, A = r(A, F, O, I, y, 5, l[20]), F, O, w, 9, l[21]), A, F, C, 14, l[22]), I, A, m, 20, l[23]), O = r(O, I = r(I, A = r(A, F, O, I, _, 5, l[24]), F, O, S, 9, l[25]), A, F, d, 14, l[26]), I, A, b, 20, l[27]), O = r(O, I = r(I, A = r(A, F, O, I, B, 5, l[28]), F, O, h, 9, l[29]), A, F, v, 14, l[30]), I, A, k, 20, l[31]), O = o(O, I = o(I, A = o(A, F, O, I, y, 4, l[32]), F, O, b, 11, l[33]), A, F, x, 16, l[34]), I, A, S, 23, l[35]), O = o(O, I = o(I, A = o(A, F, O, I, p, 4, l[36]), F, O, m, 11, l[37]), A, F, v, 16, l[38]), I, A, w, 23, l[39]), O = o(O, I = o(I, A = o(A, F, O, I, B, 4, l[40]), F, O, f, 11, l[41]), A, F, d, 16, l[42]), I, A, g, 23, l[43]), O = o(O, I = o(I, A = o(A, F, O, I, _, 4, l[44]), F, O, k, 11, l[45]), A, F, C, 16, l[46]), I, A, h, 23, l[47]), O = i(O, I = i(I, A = i(A, F, O, I, f, 6, l[48]), F, O, v, 10, l[49]), A, F, S, 15, l[50]), I, A, y, 21, l[51]), O = i(O, I = i(I, A = i(A, F, O, I, k, 6, l[52]), F, O, d, 10, l[53]), A, F, w, 15, l[54]), I, A, p, 21, l[55]), O = i(O, I = i(I, A = i(A, F, O, I, b, 6, l[56]), F, O, C, 10, l[57]), A, F, g, 15, l[58]), I, A, B, 21, l[59]), O = i(O, I = i(I, A = i(A, F, O, I, m, 6, l[60]), F, O, x, 10, l[61]), A, F, h, 15, l[62]), I, A, _, 21, l[63]), u[0] = u[0] + A | 0, u[1] = u[1] + F | 0, u[2] = u[2] + O | 0, u[3] = u[3] + I | 0;
          }, _doFinalize: function () {
            var t = this._data,
                n = t.words,
                r = 8 * this._nDataBytes,
                o = 8 * t.sigBytes;n[o >>> 5] |= 128 << 24 - o % 32;var i = e.floor(r / 4294967296),
                a = r;n[15 + (o + 64 >>> 9 << 4)] = 16711935 & (i << 8 | i >>> 24) | 4278255360 & (i << 24 | i >>> 8), n[14 + (o + 64 >>> 9 << 4)] = 16711935 & (a << 8 | a >>> 24) | 4278255360 & (a << 24 | a >>> 8), t.sigBytes = 4 * (n.length + 1), this._process();for (var s = this._hash, c = s.words, u = 0; u < 4; u++) {
              var f = c[u];c[u] = 16711935 & (f << 8 | f >>> 24) | 4278255360 & (f << 24 | f >>> 8);
            }return s;
          }, clone: function () {
            var t = u.clone.call(this);return t._hash = this._hash.clone(), t;
          } });a.MD5 = u._createHelper(p), a.HmacMD5 = u._createHmacHelper(p);
      }(Math), t.MD5;
    });
  }, { "./core": 52 }], 61: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.mode.CFB = function () {
        function e(t, e, n, r) {
          var o = this._iv;if (o) {
            i = o.slice(0);this._iv = void 0;
          } else var i = this._prevBlock;r.encryptBlock(i, 0);for (var a = 0; a < n; a++) t[e + a] ^= i[a];
        }var n = t.lib.BlockCipherMode.extend();return n.Encryptor = n.extend({ processBlock: function (t, n) {
            var r = this._cipher,
                o = r.blockSize;e.call(this, t, n, o, r), this._prevBlock = t.slice(n, n + o);
          } }), n.Decryptor = n.extend({ processBlock: function (t, n) {
            var r = this._cipher,
                o = r.blockSize,
                i = t.slice(n, n + o);e.call(this, t, n, o, r), this._prevBlock = i;
          } }), n;
      }(), t.mode.CFB;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 62: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.mode.CTRGladman = function () {
        function e(t) {
          if (255 == (t >> 24 & 255)) {
            var e = t >> 16 & 255,
                n = t >> 8 & 255,
                r = 255 & t;255 === e ? (e = 0, 255 === n ? (n = 0, 255 === r ? r = 0 : ++r) : ++n) : ++e, t = 0, t += e << 16, t += n << 8, t += r;
          } else t += 1 << 24;return t;
        }function n(t) {
          return 0 === (t[0] = e(t[0])) && (t[1] = e(t[1])), t;
        }var r = t.lib.BlockCipherMode.extend(),
            o = r.Encryptor = r.extend({ processBlock: function (t, e) {
            var r = this._cipher,
                o = r.blockSize,
                i = this._iv,
                a = this._counter;i && (a = this._counter = i.slice(0), this._iv = void 0), n(a);var s = a.slice(0);r.encryptBlock(s, 0);for (var c = 0; c < o; c++) t[e + c] ^= s[c];
          } });return r.Decryptor = o, r;
      }(), t.mode.CTRGladman;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 63: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.mode.CTR = function () {
        var e = t.lib.BlockCipherMode.extend(),
            n = e.Encryptor = e.extend({ processBlock: function (t, e) {
            var n = this._cipher,
                r = n.blockSize,
                o = this._iv,
                i = this._counter;o && (i = this._counter = o.slice(0), this._iv = void 0);var a = i.slice(0);n.encryptBlock(a, 0), i[r - 1] = i[r - 1] + 1 | 0;for (var s = 0; s < r; s++) t[e + s] ^= a[s];
          } });return e.Decryptor = n, e;
      }(), t.mode.CTR;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 64: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.mode.ECB = function () {
        var e = t.lib.BlockCipherMode.extend();return e.Encryptor = e.extend({ processBlock: function (t, e) {
            this._cipher.encryptBlock(t, e);
          } }), e.Decryptor = e.extend({ processBlock: function (t, e) {
            this._cipher.decryptBlock(t, e);
          } }), e;
      }(), t.mode.ECB;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 65: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.mode.OFB = function () {
        var e = t.lib.BlockCipherMode.extend(),
            n = e.Encryptor = e.extend({ processBlock: function (t, e) {
            var n = this._cipher,
                r = n.blockSize,
                o = this._iv,
                i = this._keystream;o && (i = this._keystream = o.slice(0), this._iv = void 0), n.encryptBlock(i, 0);for (var a = 0; a < r; a++) t[e + a] ^= i[a];
          } });return e.Decryptor = n, e;
      }(), t.mode.OFB;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 66: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.pad.AnsiX923 = { pad: function (t, e) {
          var n = t.sigBytes,
              r = 4 * e,
              o = r - n % r,
              i = n + o - 1;t.clamp(), t.words[i >>> 2] |= o << 24 - i % 4 * 8, t.sigBytes += o;
        }, unpad: function (t) {
          var e = 255 & t.words[t.sigBytes - 1 >>> 2];t.sigBytes -= e;
        } }, t.pad.Ansix923;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 67: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.pad.Iso10126 = { pad: function (e, n) {
          var r = 4 * n,
              o = r - e.sigBytes % r;e.concat(t.lib.WordArray.random(o - 1)).concat(t.lib.WordArray.create([o << 24], 1));
        }, unpad: function (t) {
          var e = 255 & t.words[t.sigBytes - 1 >>> 2];t.sigBytes -= e;
        } }, t.pad.Iso10126;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 68: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.pad.Iso97971 = { pad: function (e, n) {
          e.concat(t.lib.WordArray.create([2147483648], 1)), t.pad.ZeroPadding.pad(e, n);
        }, unpad: function (e) {
          t.pad.ZeroPadding.unpad(e), e.sigBytes--;
        } }, t.pad.Iso97971;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 69: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.pad.NoPadding = { pad: function () {}, unpad: function () {} }, t.pad.NoPadding;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 70: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return t.pad.ZeroPadding = { pad: function (t, e) {
          var n = 4 * e;t.clamp(), t.sigBytes += n - (t.sigBytes % n || n);
        }, unpad: function (t) {
          for (var e = t.words, n = t.sigBytes - 1; !(e[n >>> 2] >>> 24 - n % 4 * 8 & 255);) n--;t.sigBytes = n + 1;
        } }, t.pad.ZeroPadding;
    });
  }, { "./cipher-core": 51, "./core": 52 }], 71: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./sha1"), t("./hmac")) : "function" == typeof define && define.amd ? define(["./core", "./sha1", "./hmac"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.lib,
            r = n.Base,
            o = n.WordArray,
            i = e.algo,
            a = i.SHA1,
            s = i.HMAC,
            c = i.PBKDF2 = r.extend({ cfg: r.extend({ keySize: 4, hasher: a, iterations: 1 }), init: function (t) {
            this.cfg = this.cfg.extend(t);
          }, compute: function (t, e) {
            for (var n = this.cfg, r = s.create(n.hasher, t), i = o.create(), a = o.create([1]), c = i.words, u = a.words, f = n.keySize, l = n.iterations; c.length < f;) {
              var p = r.update(e).finalize(a);r.reset();for (var h = p.words, d = h.length, m = p, y = 1; y < l; y++) {
                m = r.finalize(m), r.reset();for (var g = m.words, v = 0; v < d; v++) h[v] ^= g[v];
              }i.concat(p), u[0]++;
            }return i.sigBytes = 4 * f, i;
          } });e.PBKDF2 = function (t, e, n) {
          return c.create(n).compute(t, e);
        };
      }(), t.PBKDF2;
    });
  }, { "./core": 52, "./hmac": 57, "./sha1": 76 }], 72: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./enc-base64"), t("./md5"), t("./evpkdf"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./enc-base64", "./md5", "./evpkdf", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e() {
          for (var t = this._X, e = this._C, n = 0; n < 8; n++) i[n] = e[n];e[0] = e[0] + 1295307597 + this._b | 0, e[1] = e[1] + 3545052371 + (e[0] >>> 0 < i[0] >>> 0 ? 1 : 0) | 0, e[2] = e[2] + 886263092 + (e[1] >>> 0 < i[1] >>> 0 ? 1 : 0) | 0, e[3] = e[3] + 1295307597 + (e[2] >>> 0 < i[2] >>> 0 ? 1 : 0) | 0, e[4] = e[4] + 3545052371 + (e[3] >>> 0 < i[3] >>> 0 ? 1 : 0) | 0, e[5] = e[5] + 886263092 + (e[4] >>> 0 < i[4] >>> 0 ? 1 : 0) | 0, e[6] = e[6] + 1295307597 + (e[5] >>> 0 < i[5] >>> 0 ? 1 : 0) | 0, e[7] = e[7] + 3545052371 + (e[6] >>> 0 < i[6] >>> 0 ? 1 : 0) | 0, this._b = e[7] >>> 0 < i[7] >>> 0 ? 1 : 0;for (n = 0; n < 8; n++) {
            var r = t[n] + e[n],
                o = 65535 & r,
                s = r >>> 16,
                c = ((o * o >>> 17) + o * s >>> 15) + s * s,
                u = ((4294901760 & r) * r | 0) + ((65535 & r) * r | 0);a[n] = c ^ u;
          }t[0] = a[0] + (a[7] << 16 | a[7] >>> 16) + (a[6] << 16 | a[6] >>> 16) | 0, t[1] = a[1] + (a[0] << 8 | a[0] >>> 24) + a[7] | 0, t[2] = a[2] + (a[1] << 16 | a[1] >>> 16) + (a[0] << 16 | a[0] >>> 16) | 0, t[3] = a[3] + (a[2] << 8 | a[2] >>> 24) + a[1] | 0, t[4] = a[4] + (a[3] << 16 | a[3] >>> 16) + (a[2] << 16 | a[2] >>> 16) | 0, t[5] = a[5] + (a[4] << 8 | a[4] >>> 24) + a[3] | 0, t[6] = a[6] + (a[5] << 16 | a[5] >>> 16) + (a[4] << 16 | a[4] >>> 16) | 0, t[7] = a[7] + (a[6] << 8 | a[6] >>> 24) + a[5] | 0;
        }var n = t,
            r = n.lib.StreamCipher,
            o = [],
            i = [],
            a = [],
            s = n.algo.RabbitLegacy = r.extend({ _doReset: function () {
            var t = this._key.words,
                n = this.cfg.iv,
                r = this._X = [t[0], t[3] << 16 | t[2] >>> 16, t[1], t[0] << 16 | t[3] >>> 16, t[2], t[1] << 16 | t[0] >>> 16, t[3], t[2] << 16 | t[1] >>> 16],
                o = this._C = [t[2] << 16 | t[2] >>> 16, 4294901760 & t[0] | 65535 & t[1], t[3] << 16 | t[3] >>> 16, 4294901760 & t[1] | 65535 & t[2], t[0] << 16 | t[0] >>> 16, 4294901760 & t[2] | 65535 & t[3], t[1] << 16 | t[1] >>> 16, 4294901760 & t[3] | 65535 & t[0]];this._b = 0;for (p = 0; p < 4; p++) e.call(this);for (p = 0; p < 8; p++) o[p] ^= r[p + 4 & 7];if (n) {
              var i = n.words,
                  a = i[0],
                  s = i[1],
                  c = 16711935 & (a << 8 | a >>> 24) | 4278255360 & (a << 24 | a >>> 8),
                  u = 16711935 & (s << 8 | s >>> 24) | 4278255360 & (s << 24 | s >>> 8),
                  f = c >>> 16 | 4294901760 & u,
                  l = u << 16 | 65535 & c;o[0] ^= c, o[1] ^= f, o[2] ^= u, o[3] ^= l, o[4] ^= c, o[5] ^= f, o[6] ^= u, o[7] ^= l;for (var p = 0; p < 4; p++) e.call(this);
            }
          }, _doProcessBlock: function (t, n) {
            var r = this._X;e.call(this), o[0] = r[0] ^ r[5] >>> 16 ^ r[3] << 16, o[1] = r[2] ^ r[7] >>> 16 ^ r[5] << 16, o[2] = r[4] ^ r[1] >>> 16 ^ r[7] << 16, o[3] = r[6] ^ r[3] >>> 16 ^ r[1] << 16;for (var i = 0; i < 4; i++) o[i] = 16711935 & (o[i] << 8 | o[i] >>> 24) | 4278255360 & (o[i] << 24 | o[i] >>> 8), t[n + i] ^= o[i];
          }, blockSize: 4, ivSize: 2 });n.RabbitLegacy = r._createHelper(s);
      }(), t.RabbitLegacy;
    });
  }, { "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./evpkdf": 55, "./md5": 60 }], 73: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./enc-base64"), t("./md5"), t("./evpkdf"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./enc-base64", "./md5", "./evpkdf", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e() {
          for (var t = this._X, e = this._C, n = 0; n < 8; n++) i[n] = e[n];e[0] = e[0] + 1295307597 + this._b | 0, e[1] = e[1] + 3545052371 + (e[0] >>> 0 < i[0] >>> 0 ? 1 : 0) | 0, e[2] = e[2] + 886263092 + (e[1] >>> 0 < i[1] >>> 0 ? 1 : 0) | 0, e[3] = e[3] + 1295307597 + (e[2] >>> 0 < i[2] >>> 0 ? 1 : 0) | 0, e[4] = e[4] + 3545052371 + (e[3] >>> 0 < i[3] >>> 0 ? 1 : 0) | 0, e[5] = e[5] + 886263092 + (e[4] >>> 0 < i[4] >>> 0 ? 1 : 0) | 0, e[6] = e[6] + 1295307597 + (e[5] >>> 0 < i[5] >>> 0 ? 1 : 0) | 0, e[7] = e[7] + 3545052371 + (e[6] >>> 0 < i[6] >>> 0 ? 1 : 0) | 0, this._b = e[7] >>> 0 < i[7] >>> 0 ? 1 : 0;for (n = 0; n < 8; n++) {
            var r = t[n] + e[n],
                o = 65535 & r,
                s = r >>> 16,
                c = ((o * o >>> 17) + o * s >>> 15) + s * s,
                u = ((4294901760 & r) * r | 0) + ((65535 & r) * r | 0);a[n] = c ^ u;
          }t[0] = a[0] + (a[7] << 16 | a[7] >>> 16) + (a[6] << 16 | a[6] >>> 16) | 0, t[1] = a[1] + (a[0] << 8 | a[0] >>> 24) + a[7] | 0, t[2] = a[2] + (a[1] << 16 | a[1] >>> 16) + (a[0] << 16 | a[0] >>> 16) | 0, t[3] = a[3] + (a[2] << 8 | a[2] >>> 24) + a[1] | 0, t[4] = a[4] + (a[3] << 16 | a[3] >>> 16) + (a[2] << 16 | a[2] >>> 16) | 0, t[5] = a[5] + (a[4] << 8 | a[4] >>> 24) + a[3] | 0, t[6] = a[6] + (a[5] << 16 | a[5] >>> 16) + (a[4] << 16 | a[4] >>> 16) | 0, t[7] = a[7] + (a[6] << 8 | a[6] >>> 24) + a[5] | 0;
        }var n = t,
            r = n.lib.StreamCipher,
            o = [],
            i = [],
            a = [],
            s = n.algo.Rabbit = r.extend({ _doReset: function () {
            for (var t = this._key.words, n = this.cfg.iv, r = 0; r < 4; r++) t[r] = 16711935 & (t[r] << 8 | t[r] >>> 24) | 4278255360 & (t[r] << 24 | t[r] >>> 8);var o = this._X = [t[0], t[3] << 16 | t[2] >>> 16, t[1], t[0] << 16 | t[3] >>> 16, t[2], t[1] << 16 | t[0] >>> 16, t[3], t[2] << 16 | t[1] >>> 16],
                i = this._C = [t[2] << 16 | t[2] >>> 16, 4294901760 & t[0] | 65535 & t[1], t[3] << 16 | t[3] >>> 16, 4294901760 & t[1] | 65535 & t[2], t[0] << 16 | t[0] >>> 16, 4294901760 & t[2] | 65535 & t[3], t[1] << 16 | t[1] >>> 16, 4294901760 & t[3] | 65535 & t[0]];this._b = 0;for (r = 0; r < 4; r++) e.call(this);for (r = 0; r < 8; r++) i[r] ^= o[r + 4 & 7];if (n) {
              var a = n.words,
                  s = a[0],
                  c = a[1],
                  u = 16711935 & (s << 8 | s >>> 24) | 4278255360 & (s << 24 | s >>> 8),
                  f = 16711935 & (c << 8 | c >>> 24) | 4278255360 & (c << 24 | c >>> 8),
                  l = u >>> 16 | 4294901760 & f,
                  p = f << 16 | 65535 & u;i[0] ^= u, i[1] ^= l, i[2] ^= f, i[3] ^= p, i[4] ^= u, i[5] ^= l, i[6] ^= f, i[7] ^= p;for (r = 0; r < 4; r++) e.call(this);
            }
          }, _doProcessBlock: function (t, n) {
            var r = this._X;e.call(this), o[0] = r[0] ^ r[5] >>> 16 ^ r[3] << 16, o[1] = r[2] ^ r[7] >>> 16 ^ r[5] << 16, o[2] = r[4] ^ r[1] >>> 16 ^ r[7] << 16, o[3] = r[6] ^ r[3] >>> 16 ^ r[1] << 16;for (var i = 0; i < 4; i++) o[i] = 16711935 & (o[i] << 8 | o[i] >>> 24) | 4278255360 & (o[i] << 24 | o[i] >>> 8), t[n + i] ^= o[i];
          }, blockSize: 4, ivSize: 2 });n.Rabbit = r._createHelper(s);
      }(), t.Rabbit;
    });
  }, { "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./evpkdf": 55, "./md5": 60 }], 74: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./enc-base64"), t("./md5"), t("./evpkdf"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./enc-base64", "./md5", "./evpkdf", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e() {
          for (var t = this._S, e = this._i, n = this._j, r = 0, o = 0; o < 4; o++) {
            n = (n + t[e = (e + 1) % 256]) % 256;var i = t[e];t[e] = t[n], t[n] = i, r |= t[(t[e] + t[n]) % 256] << 24 - 8 * o;
          }return this._i = e, this._j = n, r;
        }var n = t,
            r = n.lib.StreamCipher,
            o = n.algo,
            i = o.RC4 = r.extend({ _doReset: function () {
            for (var t = this._key, e = t.words, n = t.sigBytes, r = this._S = [], o = 0; o < 256; o++) r[o] = o;for (var o = 0, i = 0; o < 256; o++) {
              var a = o % n,
                  s = e[a >>> 2] >>> 24 - a % 4 * 8 & 255;i = (i + r[o] + s) % 256;var c = r[o];r[o] = r[i], r[i] = c;
            }this._i = this._j = 0;
          }, _doProcessBlock: function (t, n) {
            t[n] ^= e.call(this);
          }, keySize: 8, ivSize: 0 });n.RC4 = r._createHelper(i);var a = o.RC4Drop = i.extend({ cfg: i.cfg.extend({ drop: 192 }), _doReset: function () {
            i._doReset.call(this);for (var t = this.cfg.drop; t > 0; t--) e.call(this);
          } });n.RC4Drop = r._createHelper(a);
      }(), t.RC4;
    });
  }, { "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./evpkdf": 55, "./md5": 60 }], 75: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        function n(t, e, n) {
          return t ^ e ^ n;
        }function r(t, e, n) {
          return t & e | ~t & n;
        }function o(t, e, n) {
          return (t | ~e) ^ n;
        }function i(t, e, n) {
          return t & n | e & ~n;
        }function a(t, e, n) {
          return t ^ (e | ~n);
        }function s(t, e) {
          return t << e | t >>> 32 - e;
        }var c = t,
            u = c.lib,
            f = u.WordArray,
            l = u.Hasher,
            p = c.algo,
            h = f.create([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8, 3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12, 1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2, 4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13]),
            d = f.create([5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12, 6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2, 15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13, 8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14, 12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11]),
            m = f.create([11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8, 7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12, 11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5, 11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12, 9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6]),
            y = f.create([8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6, 9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11, 9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5, 15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8, 8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11]),
            g = f.create([0, 1518500249, 1859775393, 2400959708, 2840853838]),
            v = f.create([1352829926, 1548603684, 1836072691, 2053994217, 0]),
            b = p.RIPEMD160 = l.extend({ _doReset: function () {
            this._hash = f.create([1732584193, 4023233417, 2562383102, 271733878, 3285377520]);
          }, _doProcessBlock: function (t, e) {
            for (D = 0; D < 16; D++) {
              var c = e + D,
                  u = t[c];t[c] = 16711935 & (u << 8 | u >>> 24) | 4278255360 & (u << 24 | u >>> 8);
            }var f,
                l,
                p,
                b,
                _,
                w,
                x,
                k,
                B,
                S,
                C = this._hash.words,
                A = g.words,
                F = v.words,
                O = h.words,
                I = d.words,
                N = m.words,
                T = y.words;w = f = C[0], x = l = C[1], k = p = C[2], B = b = C[3], S = _ = C[4];for (var P, D = 0; D < 80; D += 1) P = f + t[e + O[D]] | 0, P += D < 16 ? n(l, p, b) + A[0] : D < 32 ? r(l, p, b) + A[1] : D < 48 ? o(l, p, b) + A[2] : D < 64 ? i(l, p, b) + A[3] : a(l, p, b) + A[4], P = (P = s(P |= 0, N[D])) + _ | 0, f = _, _ = b, b = s(p, 10), p = l, l = P, P = w + t[e + I[D]] | 0, P += D < 16 ? a(x, k, B) + F[0] : D < 32 ? i(x, k, B) + F[1] : D < 48 ? o(x, k, B) + F[2] : D < 64 ? r(x, k, B) + F[3] : n(x, k, B) + F[4], P = (P = s(P |= 0, T[D])) + S | 0, w = S, S = B, B = s(k, 10), k = x, x = P;P = C[1] + p + B | 0, C[1] = C[2] + b + S | 0, C[2] = C[3] + _ + w | 0, C[3] = C[4] + f + x | 0, C[4] = C[0] + l + k | 0, C[0] = P;
          }, _doFinalize: function () {
            var t = this._data,
                e = t.words,
                n = 8 * this._nDataBytes,
                r = 8 * t.sigBytes;e[r >>> 5] |= 128 << 24 - r % 32, e[14 + (r + 64 >>> 9 << 4)] = 16711935 & (n << 8 | n >>> 24) | 4278255360 & (n << 24 | n >>> 8), t.sigBytes = 4 * (e.length + 1), this._process();for (var o = this._hash, i = o.words, a = 0; a < 5; a++) {
              var s = i[a];i[a] = 16711935 & (s << 8 | s >>> 24) | 4278255360 & (s << 24 | s >>> 8);
            }return o;
          }, clone: function () {
            var t = l.clone.call(this);return t._hash = this._hash.clone(), t;
          } });c.RIPEMD160 = l._createHelper(b), c.HmacRIPEMD160 = l._createHmacHelper(b);
      }(Math), t.RIPEMD160;
    });
  }, { "./core": 52 }], 76: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.lib,
            r = n.WordArray,
            o = n.Hasher,
            i = [],
            a = e.algo.SHA1 = o.extend({ _doReset: function () {
            this._hash = new r.init([1732584193, 4023233417, 2562383102, 271733878, 3285377520]);
          }, _doProcessBlock: function (t, e) {
            for (var n = this._hash.words, r = n[0], o = n[1], a = n[2], s = n[3], c = n[4], u = 0; u < 80; u++) {
              if (u < 16) i[u] = 0 | t[e + u];else {
                var f = i[u - 3] ^ i[u - 8] ^ i[u - 14] ^ i[u - 16];i[u] = f << 1 | f >>> 31;
              }var l = (r << 5 | r >>> 27) + c + i[u];l += u < 20 ? 1518500249 + (o & a | ~o & s) : u < 40 ? 1859775393 + (o ^ a ^ s) : u < 60 ? (o & a | o & s | a & s) - 1894007588 : (o ^ a ^ s) - 899497514, c = s, s = a, a = o << 30 | o >>> 2, o = r, r = l;
            }n[0] = n[0] + r | 0, n[1] = n[1] + o | 0, n[2] = n[2] + a | 0, n[3] = n[3] + s | 0, n[4] = n[4] + c | 0;
          }, _doFinalize: function () {
            var t = this._data,
                e = t.words,
                n = 8 * this._nDataBytes,
                r = 8 * t.sigBytes;return e[r >>> 5] |= 128 << 24 - r % 32, e[14 + (r + 64 >>> 9 << 4)] = Math.floor(n / 4294967296), e[15 + (r + 64 >>> 9 << 4)] = n, t.sigBytes = 4 * e.length, this._process(), this._hash;
          }, clone: function () {
            var t = o.clone.call(this);return t._hash = this._hash.clone(), t;
          } });e.SHA1 = o._createHelper(a), e.HmacSHA1 = o._createHmacHelper(a);
      }(), t.SHA1;
    });
  }, { "./core": 52 }], 77: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./sha256")) : "function" == typeof define && define.amd ? define(["./core", "./sha256"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.lib.WordArray,
            r = e.algo,
            o = r.SHA256,
            i = r.SHA224 = o.extend({ _doReset: function () {
            this._hash = new n.init([3238371032, 914150663, 812702999, 4144912697, 4290775857, 1750603025, 1694076839, 3204075428]);
          }, _doFinalize: function () {
            var t = o._doFinalize.call(this);return t.sigBytes -= 4, t;
          } });e.SHA224 = o._createHelper(i), e.HmacSHA224 = o._createHmacHelper(i);
      }(), t.SHA224;
    });
  }, { "./core": 52, "./sha256": 78 }], 78: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        var n = t,
            r = n.lib,
            o = r.WordArray,
            i = r.Hasher,
            a = n.algo,
            s = [],
            c = [];!function () {
          function t(t) {
            return 4294967296 * (t - (0 | t)) | 0;
          }for (var n = 2, r = 0; r < 64;) (function (t) {
            for (var n = e.sqrt(t), r = 2; r <= n; r++) if (!(t % r)) return !1;return !0;
          })(n) && (r < 8 && (s[r] = t(e.pow(n, .5))), c[r] = t(e.pow(n, 1 / 3)), r++), n++;
        }();var u = [],
            f = a.SHA256 = i.extend({ _doReset: function () {
            this._hash = new o.init(s.slice(0));
          }, _doProcessBlock: function (t, e) {
            for (var n = this._hash.words, r = n[0], o = n[1], i = n[2], a = n[3], s = n[4], f = n[5], l = n[6], p = n[7], h = 0; h < 64; h++) {
              if (h < 16) u[h] = 0 | t[e + h];else {
                var d = u[h - 15],
                    m = (d << 25 | d >>> 7) ^ (d << 14 | d >>> 18) ^ d >>> 3,
                    y = u[h - 2],
                    g = (y << 15 | y >>> 17) ^ (y << 13 | y >>> 19) ^ y >>> 10;u[h] = m + u[h - 7] + g + u[h - 16];
              }var v = r & o ^ r & i ^ o & i,
                  b = (r << 30 | r >>> 2) ^ (r << 19 | r >>> 13) ^ (r << 10 | r >>> 22),
                  _ = p + ((s << 26 | s >>> 6) ^ (s << 21 | s >>> 11) ^ (s << 7 | s >>> 25)) + (s & f ^ ~s & l) + c[h] + u[h];p = l, l = f, f = s, s = a + _ | 0, a = i, i = o, o = r, r = _ + (b + v) | 0;
            }n[0] = n[0] + r | 0, n[1] = n[1] + o | 0, n[2] = n[2] + i | 0, n[3] = n[3] + a | 0, n[4] = n[4] + s | 0, n[5] = n[5] + f | 0, n[6] = n[6] + l | 0, n[7] = n[7] + p | 0;
          }, _doFinalize: function () {
            var t = this._data,
                n = t.words,
                r = 8 * this._nDataBytes,
                o = 8 * t.sigBytes;return n[o >>> 5] |= 128 << 24 - o % 32, n[14 + (o + 64 >>> 9 << 4)] = e.floor(r / 4294967296), n[15 + (o + 64 >>> 9 << 4)] = r, t.sigBytes = 4 * n.length, this._process(), this._hash;
          }, clone: function () {
            var t = i.clone.call(this);return t._hash = this._hash.clone(), t;
          } });n.SHA256 = i._createHelper(f), n.HmacSHA256 = i._createHmacHelper(f);
      }(Math), t.SHA256;
    });
  }, { "./core": 52 }], 79: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./x64-core")) : "function" == typeof define && define.amd ? define(["./core", "./x64-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        var n = t,
            r = n.lib,
            o = r.WordArray,
            i = r.Hasher,
            a = n.x64.Word,
            s = n.algo,
            c = [],
            u = [],
            f = [];!function () {
          for (var t = 1, e = 0, n = 0; n < 24; n++) {
            c[t + 5 * e] = (n + 1) * (n + 2) / 2 % 64;var r = (2 * t + 3 * e) % 5;t = e % 5, e = r;
          }for (t = 0; t < 5; t++) for (e = 0; e < 5; e++) u[t + 5 * e] = e + (2 * t + 3 * e) % 5 * 5;for (var o = 1, i = 0; i < 24; i++) {
            for (var s = 0, l = 0, p = 0; p < 7; p++) {
              if (1 & o) {
                var h = (1 << p) - 1;h < 32 ? l ^= 1 << h : s ^= 1 << h - 32;
              }128 & o ? o = o << 1 ^ 113 : o <<= 1;
            }f[i] = a.create(s, l);
          }
        }();var l = [];!function () {
          for (var t = 0; t < 25; t++) l[t] = a.create();
        }();var p = s.SHA3 = i.extend({ cfg: i.cfg.extend({ outputLength: 512 }), _doReset: function () {
            for (var t = this._state = [], e = 0; e < 25; e++) t[e] = new a.init();this.blockSize = (1600 - 2 * this.cfg.outputLength) / 32;
          }, _doProcessBlock: function (t, e) {
            for (var n = this._state, r = this.blockSize / 2, o = 0; o < r; o++) {
              var i = t[e + 2 * o],
                  a = t[e + 2 * o + 1];i = 16711935 & (i << 8 | i >>> 24) | 4278255360 & (i << 24 | i >>> 8), a = 16711935 & (a << 8 | a >>> 24) | 4278255360 & (a << 24 | a >>> 8), (F = n[o]).high ^= a, F.low ^= i;
            }for (var s = 0; s < 24; s++) {
              for (A = 0; A < 5; A++) {
                for (var p = 0, h = 0, d = 0; d < 5; d++) p ^= (F = n[A + 5 * d]).high, h ^= F.low;var m = l[A];m.high = p, m.low = h;
              }for (A = 0; A < 5; A++) for (var y = l[(A + 4) % 5], g = l[(A + 1) % 5], v = g.high, b = g.low, p = y.high ^ (v << 1 | b >>> 31), h = y.low ^ (b << 1 | v >>> 31), d = 0; d < 5; d++) (F = n[A + 5 * d]).high ^= p, F.low ^= h;for (var _ = 1; _ < 25; _++) {
                var w = (F = n[_]).high,
                    x = F.low,
                    k = c[_];if (k < 32) var p = w << k | x >>> 32 - k,
                    h = x << k | w >>> 32 - k;else var p = x << k - 32 | w >>> 64 - k,
                    h = w << k - 32 | x >>> 64 - k;var B = l[u[_]];B.high = p, B.low = h;
              }var S = l[0],
                  C = n[0];S.high = C.high, S.low = C.low;for (var A = 0; A < 5; A++) for (d = 0; d < 5; d++) {
                var F = n[_ = A + 5 * d],
                    O = l[_],
                    I = l[(A + 1) % 5 + 5 * d],
                    N = l[(A + 2) % 5 + 5 * d];F.high = O.high ^ ~I.high & N.high, F.low = O.low ^ ~I.low & N.low;
              }var F = n[0],
                  T = f[s];F.high ^= T.high, F.low ^= T.low;
            }
          }, _doFinalize: function () {
            var t = this._data,
                n = t.words,
                r = (this._nDataBytes, 8 * t.sigBytes),
                i = 32 * this.blockSize;n[r >>> 5] |= 1 << 24 - r % 32, n[(e.ceil((r + 1) / i) * i >>> 5) - 1] |= 128, t.sigBytes = 4 * n.length, this._process();for (var a = this._state, s = this.cfg.outputLength / 8, c = s / 8, u = [], f = 0; f < c; f++) {
              var l = a[f],
                  p = l.high,
                  h = l.low;p = 16711935 & (p << 8 | p >>> 24) | 4278255360 & (p << 24 | p >>> 8), h = 16711935 & (h << 8 | h >>> 24) | 4278255360 & (h << 24 | h >>> 8), u.push(h), u.push(p);
            }return new o.init(u, s);
          }, clone: function () {
            for (var t = i.clone.call(this), e = t._state = this._state.slice(0), n = 0; n < 25; n++) e[n] = e[n].clone();return t;
          } });n.SHA3 = i._createHelper(p), n.HmacSHA3 = i._createHmacHelper(p);
      }(Math), t.SHA3;
    });
  }, { "./core": 52, "./x64-core": 83 }], 80: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./x64-core"), t("./sha512")) : "function" == typeof define && define.amd ? define(["./core", "./x64-core", "./sha512"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        var e = t,
            n = e.x64,
            r = n.Word,
            o = n.WordArray,
            i = e.algo,
            a = i.SHA512,
            s = i.SHA384 = a.extend({ _doReset: function () {
            this._hash = new o.init([new r.init(3418070365, 3238371032), new r.init(1654270250, 914150663), new r.init(2438529370, 812702999), new r.init(355462360, 4144912697), new r.init(1731405415, 4290775857), new r.init(2394180231, 1750603025), new r.init(3675008525, 1694076839), new r.init(1203062813, 3204075428)]);
          }, _doFinalize: function () {
            var t = a._doFinalize.call(this);return t.sigBytes -= 16, t;
          } });e.SHA384 = a._createHelper(s), e.HmacSHA384 = a._createHmacHelper(s);
      }(), t.SHA384;
    });
  }, { "./core": 52, "./sha512": 81, "./x64-core": 83 }], 81: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./x64-core")) : "function" == typeof define && define.amd ? define(["./core", "./x64-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e() {
          return i.create.apply(i, arguments);
        }var n = t,
            r = n.lib.Hasher,
            o = n.x64,
            i = o.Word,
            a = o.WordArray,
            s = n.algo,
            c = [e(1116352408, 3609767458), e(1899447441, 602891725), e(3049323471, 3964484399), e(3921009573, 2173295548), e(961987163, 4081628472), e(1508970993, 3053834265), e(2453635748, 2937671579), e(2870763221, 3664609560), e(3624381080, 2734883394), e(310598401, 1164996542), e(607225278, 1323610764), e(1426881987, 3590304994), e(1925078388, 4068182383), e(2162078206, 991336113), e(2614888103, 633803317), e(3248222580, 3479774868), e(3835390401, 2666613458), e(4022224774, 944711139), e(264347078, 2341262773), e(604807628, 2007800933), e(770255983, 1495990901), e(1249150122, 1856431235), e(1555081692, 3175218132), e(1996064986, 2198950837), e(2554220882, 3999719339), e(2821834349, 766784016), e(2952996808, 2566594879), e(3210313671, 3203337956), e(3336571891, 1034457026), e(3584528711, 2466948901), e(113926993, 3758326383), e(338241895, 168717936), e(666307205, 1188179964), e(773529912, 1546045734), e(1294757372, 1522805485), e(1396182291, 2643833823), e(1695183700, 2343527390), e(1986661051, 1014477480), e(2177026350, 1206759142), e(2456956037, 344077627), e(2730485921, 1290863460), e(2820302411, 3158454273), e(3259730800, 3505952657), e(3345764771, 106217008), e(3516065817, 3606008344), e(3600352804, 1432725776), e(4094571909, 1467031594), e(275423344, 851169720), e(430227734, 3100823752), e(506948616, 1363258195), e(659060556, 3750685593), e(883997877, 3785050280), e(958139571, 3318307427), e(1322822218, 3812723403), e(1537002063, 2003034995), e(1747873779, 3602036899), e(1955562222, 1575990012), e(2024104815, 1125592928), e(2227730452, 2716904306), e(2361852424, 442776044), e(2428436474, 593698344), e(2756734187, 3733110249), e(3204031479, 2999351573), e(3329325298, 3815920427), e(3391569614, 3928383900), e(3515267271, 566280711), e(3940187606, 3454069534), e(4118630271, 4000239992), e(116418474, 1914138554), e(174292421, 2731055270), e(289380356, 3203993006), e(460393269, 320620315), e(685471733, 587496836), e(852142971, 1086792851), e(1017036298, 365543100), e(1126000580, 2618297676), e(1288033470, 3409855158), e(1501505948, 4234509866), e(1607167915, 987167468), e(1816402316, 1246189591)],
            u = [];!function () {
          for (var t = 0; t < 80; t++) u[t] = e();
        }();var f = s.SHA512 = r.extend({ _doReset: function () {
            this._hash = new a.init([new i.init(1779033703, 4089235720), new i.init(3144134277, 2227873595), new i.init(1013904242, 4271175723), new i.init(2773480762, 1595750129), new i.init(1359893119, 2917565137), new i.init(2600822924, 725511199), new i.init(528734635, 4215389547), new i.init(1541459225, 327033209)]);
          }, _doProcessBlock: function (t, e) {
            for (var n = this._hash.words, r = n[0], o = n[1], i = n[2], a = n[3], s = n[4], f = n[5], l = n[6], p = n[7], h = r.high, d = r.low, m = o.high, y = o.low, g = i.high, v = i.low, b = a.high, _ = a.low, w = s.high, x = s.low, k = f.high, B = f.low, S = l.high, C = l.low, A = p.high, F = p.low, O = h, I = d, N = m, T = y, P = g, D = v, R = b, E = _, M = w, H = x, j = k, q = B, z = S, L = C, U = A, W = F, J = 0; J < 80; J++) {
              var K = u[J];if (J < 16) var G = K.high = 0 | t[e + 2 * J],
                  X = K.low = 0 | t[e + 2 * J + 1];else {
                var $ = u[J - 15],
                    V = $.high,
                    Z = $.low,
                    Y = (V >>> 1 | Z << 31) ^ (V >>> 8 | Z << 24) ^ V >>> 7,
                    Q = (Z >>> 1 | V << 31) ^ (Z >>> 8 | V << 24) ^ (Z >>> 7 | V << 25),
                    tt = u[J - 2],
                    et = tt.high,
                    nt = tt.low,
                    rt = (et >>> 19 | nt << 13) ^ (et << 3 | nt >>> 29) ^ et >>> 6,
                    ot = (nt >>> 19 | et << 13) ^ (nt << 3 | et >>> 29) ^ (nt >>> 6 | et << 26),
                    it = u[J - 7],
                    at = it.high,
                    st = it.low,
                    ct = u[J - 16],
                    ut = ct.high,
                    ft = ct.low,
                    G = (G = (G = Y + at + ((X = Q + st) >>> 0 < Q >>> 0 ? 1 : 0)) + rt + ((X = X + ot) >>> 0 < ot >>> 0 ? 1 : 0)) + ut + ((X = X + ft) >>> 0 < ft >>> 0 ? 1 : 0);K.high = G, K.low = X;
              }var lt = M & j ^ ~M & z,
                  pt = H & q ^ ~H & L,
                  ht = O & N ^ O & P ^ N & P,
                  dt = I & T ^ I & D ^ T & D,
                  mt = (O >>> 28 | I << 4) ^ (O << 30 | I >>> 2) ^ (O << 25 | I >>> 7),
                  yt = (I >>> 28 | O << 4) ^ (I << 30 | O >>> 2) ^ (I << 25 | O >>> 7),
                  gt = (M >>> 14 | H << 18) ^ (M >>> 18 | H << 14) ^ (M << 23 | H >>> 9),
                  vt = (H >>> 14 | M << 18) ^ (H >>> 18 | M << 14) ^ (H << 23 | M >>> 9),
                  bt = c[J],
                  _t = bt.high,
                  wt = bt.low,
                  xt = W + vt,
                  kt = (kt = (kt = (kt = U + gt + (xt >>> 0 < W >>> 0 ? 1 : 0)) + lt + ((xt = xt + pt) >>> 0 < pt >>> 0 ? 1 : 0)) + _t + ((xt = xt + wt) >>> 0 < wt >>> 0 ? 1 : 0)) + G + ((xt = xt + X) >>> 0 < X >>> 0 ? 1 : 0),
                  Bt = yt + dt,
                  St = mt + ht + (Bt >>> 0 < yt >>> 0 ? 1 : 0);U = z, W = L, z = j, L = q, j = M, q = H, M = R + kt + ((H = E + xt | 0) >>> 0 < E >>> 0 ? 1 : 0) | 0, R = P, E = D, P = N, D = T, N = O, T = I, O = kt + St + ((I = xt + Bt | 0) >>> 0 < xt >>> 0 ? 1 : 0) | 0;
            }d = r.low = d + I, r.high = h + O + (d >>> 0 < I >>> 0 ? 1 : 0), y = o.low = y + T, o.high = m + N + (y >>> 0 < T >>> 0 ? 1 : 0), v = i.low = v + D, i.high = g + P + (v >>> 0 < D >>> 0 ? 1 : 0), _ = a.low = _ + E, a.high = b + R + (_ >>> 0 < E >>> 0 ? 1 : 0), x = s.low = x + H, s.high = w + M + (x >>> 0 < H >>> 0 ? 1 : 0), B = f.low = B + q, f.high = k + j + (B >>> 0 < q >>> 0 ? 1 : 0), C = l.low = C + L, l.high = S + z + (C >>> 0 < L >>> 0 ? 1 : 0), F = p.low = F + W, p.high = A + U + (F >>> 0 < W >>> 0 ? 1 : 0);
          }, _doFinalize: function () {
            var t = this._data,
                e = t.words,
                n = 8 * this._nDataBytes,
                r = 8 * t.sigBytes;return e[r >>> 5] |= 128 << 24 - r % 32, e[30 + (r + 128 >>> 10 << 5)] = Math.floor(n / 4294967296), e[31 + (r + 128 >>> 10 << 5)] = n, t.sigBytes = 4 * e.length, this._process(), this._hash.toX32();
          }, clone: function () {
            var t = r.clone.call(this);return t._hash = this._hash.clone(), t;
          }, blockSize: 32 });n.SHA512 = r._createHelper(f), n.HmacSHA512 = r._createHmacHelper(f);
      }(), t.SHA512;
    });
  }, { "./core": 52, "./x64-core": 83 }], 82: [function (t, e, n) {
    !function (r, o, i) {
      "object" == typeof n ? e.exports = n = o(t("./core"), t("./enc-base64"), t("./md5"), t("./evpkdf"), t("./cipher-core")) : "function" == typeof define && define.amd ? define(["./core", "./enc-base64", "./md5", "./evpkdf", "./cipher-core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function () {
        function e(t, e) {
          var n = (this._lBlock >>> t ^ this._rBlock) & e;this._rBlock ^= n, this._lBlock ^= n << t;
        }function n(t, e) {
          var n = (this._rBlock >>> t ^ this._lBlock) & e;this._lBlock ^= n, this._rBlock ^= n << t;
        }var r = t,
            o = r.lib,
            i = o.WordArray,
            a = o.BlockCipher,
            s = r.algo,
            c = [57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36, 63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4],
            u = [14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10, 23, 19, 12, 4, 26, 8, 16, 7, 27, 20, 13, 2, 41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32],
            f = [1, 2, 4, 6, 8, 10, 12, 14, 15, 17, 19, 21, 23, 25, 27, 28],
            l = [{ 0: 8421888, 268435456: 32768, 536870912: 8421378, 805306368: 2, 1073741824: 512, 1342177280: 8421890, 1610612736: 8389122, 1879048192: 8388608, 2147483648: 514, 2415919104: 8389120, 2684354560: 33280, 2952790016: 8421376, 3221225472: 32770, 3489660928: 8388610, 3758096384: 0, 4026531840: 33282, 134217728: 0, 402653184: 8421890, 671088640: 33282, 939524096: 32768, 1207959552: 8421888, 1476395008: 512, 1744830464: 8421378, 2013265920: 2, 2281701376: 8389120, 2550136832: 33280, 2818572288: 8421376, 3087007744: 8389122, 3355443200: 8388610, 3623878656: 32770, 3892314112: 514, 4160749568: 8388608, 1: 32768, 268435457: 2, 536870913: 8421888, 805306369: 8388608, 1073741825: 8421378, 1342177281: 33280, 1610612737: 512, 1879048193: 8389122, 2147483649: 8421890, 2415919105: 8421376, 2684354561: 8388610, 2952790017: 33282, 3221225473: 514, 3489660929: 8389120, 3758096385: 32770, 4026531841: 0, 134217729: 8421890, 402653185: 8421376, 671088641: 8388608, 939524097: 512, 1207959553: 32768, 1476395009: 8388610, 1744830465: 2, 2013265921: 33282, 2281701377: 32770, 2550136833: 8389122, 2818572289: 514, 3087007745: 8421888, 3355443201: 8389120, 3623878657: 0, 3892314113: 33280, 4160749569: 8421378 }, { 0: 1074282512, 16777216: 16384, 33554432: 524288, 50331648: 1074266128, 67108864: 1073741840, 83886080: 1074282496, 100663296: 1073758208, 117440512: 16, 134217728: 540672, 150994944: 1073758224, 167772160: 1073741824, 184549376: 540688, 201326592: 524304, 218103808: 0, 234881024: 16400, 251658240: 1074266112, 8388608: 1073758208, 25165824: 540688, 41943040: 16, 58720256: 1073758224, 75497472: 1074282512, 92274688: 1073741824, 109051904: 524288, 125829120: 1074266128, 142606336: 524304, 159383552: 0, 176160768: 16384, 192937984: 1074266112, 209715200: 1073741840, 226492416: 540672, 243269632: 1074282496, 260046848: 16400, 268435456: 0, 285212672: 1074266128, 301989888: 1073758224, 318767104: 1074282496, 335544320: 1074266112, 352321536: 16, 369098752: 540688, 385875968: 16384, 402653184: 16400, 419430400: 524288, 436207616: 524304, 452984832: 1073741840, 469762048: 540672, 486539264: 1073758208, 503316480: 1073741824, 520093696: 1074282512, 276824064: 540688, 293601280: 524288, 310378496: 1074266112, 327155712: 16384, 343932928: 1073758208, 360710144: 1074282512, 377487360: 16, 394264576: 1073741824, 411041792: 1074282496, 427819008: 1073741840, 444596224: 1073758224, 461373440: 524304, 478150656: 0, 494927872: 16400, 511705088: 1074266128, 528482304: 540672 }, { 0: 260, 1048576: 0, 2097152: 67109120, 3145728: 65796, 4194304: 65540, 5242880: 67108868, 6291456: 67174660, 7340032: 67174400, 8388608: 67108864, 9437184: 67174656, 10485760: 65792, 11534336: 67174404, 12582912: 67109124, 13631488: 65536, 14680064: 4, 15728640: 256, 524288: 67174656, 1572864: 67174404, 2621440: 0, 3670016: 67109120, 4718592: 67108868, 5767168: 65536, 6815744: 65540, 7864320: 260, 8912896: 4, 9961472: 256, 11010048: 67174400, 12058624: 65796, 13107200: 65792, 14155776: 67109124, 15204352: 67174660, 16252928: 67108864, 16777216: 67174656, 17825792: 65540, 18874368: 65536, 19922944: 67109120, 20971520: 256, 22020096: 67174660, 23068672: 67108868, 24117248: 0, 25165824: 67109124, 26214400: 67108864, 27262976: 4, 28311552: 65792, 29360128: 67174400, 30408704: 260, 31457280: 65796, 32505856: 67174404, 17301504: 67108864, 18350080: 260, 19398656: 67174656, 20447232: 0, 21495808: 65540, 22544384: 67109120, 23592960: 256, 24641536: 67174404, 25690112: 65536, 26738688: 67174660, 27787264: 65796, 28835840: 67108868, 29884416: 67109124, 30932992: 67174400, 31981568: 4, 33030144: 65792 }, { 0: 2151682048, 65536: 2147487808, 131072: 4198464, 196608: 2151677952, 262144: 0, 327680: 4198400, 393216: 2147483712, 458752: 4194368, 524288: 2147483648, 589824: 4194304, 655360: 64, 720896: 2147487744, 786432: 2151678016, 851968: 4160, 917504: 4096, 983040: 2151682112, 32768: 2147487808, 98304: 64, 163840: 2151678016, 229376: 2147487744, 294912: 4198400, 360448: 2151682112, 425984: 0, 491520: 2151677952, 557056: 4096, 622592: 2151682048, 688128: 4194304, 753664: 4160, 819200: 2147483648, 884736: 4194368, 950272: 4198464, 1015808: 2147483712, 1048576: 4194368, 1114112: 4198400, 1179648: 2147483712, 1245184: 0, 1310720: 4160, 1376256: 2151678016, 1441792: 2151682048, 1507328: 2147487808, 1572864: 2151682112, 1638400: 2147483648, 1703936: 2151677952, 1769472: 4198464, 1835008: 2147487744, 1900544: 4194304, 1966080: 64, 2031616: 4096, 1081344: 2151677952, 1146880: 2151682112, 1212416: 0, 1277952: 4198400, 1343488: 4194368, 1409024: 2147483648, 1474560: 2147487808, 1540096: 64, 1605632: 2147483712, 1671168: 4096, 1736704: 2147487744, 1802240: 2151678016, 1867776: 4160, 1933312: 2151682048, 1998848: 4194304, 2064384: 4198464 }, { 0: 128, 4096: 17039360, 8192: 262144, 12288: 536870912, 16384: 537133184, 20480: 16777344, 24576: 553648256, 28672: 262272, 32768: 16777216, 36864: 537133056, 40960: 536871040, 45056: 553910400, 49152: 553910272, 53248: 0, 57344: 17039488, 61440: 553648128, 2048: 17039488, 6144: 553648256, 10240: 128, 14336: 17039360, 18432: 262144, 22528: 537133184, 26624: 553910272, 30720: 536870912, 34816: 537133056, 38912: 0, 43008: 553910400, 47104: 16777344, 51200: 536871040, 55296: 553648128, 59392: 16777216, 63488: 262272, 65536: 262144, 69632: 128, 73728: 536870912, 77824: 553648256, 81920: 16777344, 86016: 553910272, 90112: 537133184, 94208: 16777216, 98304: 553910400, 102400: 553648128, 106496: 17039360, 110592: 537133056, 114688: 262272, 118784: 536871040, 122880: 0, 126976: 17039488, 67584: 553648256, 71680: 16777216, 75776: 17039360, 79872: 537133184, 83968: 536870912, 88064: 17039488, 92160: 128, 96256: 553910272, 100352: 262272, 104448: 553910400, 108544: 0, 112640: 553648128, 116736: 16777344, 120832: 262144, 124928: 537133056, 129024: 536871040 }, { 0: 268435464, 256: 8192, 512: 270532608, 768: 270540808, 1024: 268443648, 1280: 2097152, 1536: 2097160, 1792: 268435456, 2048: 0, 2304: 268443656, 2560: 2105344, 2816: 8, 3072: 270532616, 3328: 2105352, 3584: 8200, 3840: 270540800, 128: 270532608, 384: 270540808, 640: 8, 896: 2097152, 1152: 2105352, 1408: 268435464, 1664: 268443648, 1920: 8200, 2176: 2097160, 2432: 8192, 2688: 268443656, 2944: 270532616, 3200: 0, 3456: 270540800, 3712: 2105344, 3968: 268435456, 4096: 268443648, 4352: 270532616, 4608: 270540808, 4864: 8200, 5120: 2097152, 5376: 268435456, 5632: 268435464, 5888: 2105344, 6144: 2105352, 6400: 0, 6656: 8, 6912: 270532608, 7168: 8192, 7424: 268443656, 7680: 270540800, 7936: 2097160, 4224: 8, 4480: 2105344, 4736: 2097152, 4992: 268435464, 5248: 268443648, 5504: 8200, 5760: 270540808, 6016: 270532608, 6272: 270540800, 6528: 270532616, 6784: 8192, 7040: 2105352, 7296: 2097160, 7552: 0, 7808: 268435456, 8064: 268443656 }, { 0: 1048576, 16: 33555457, 32: 1024, 48: 1049601, 64: 34604033, 80: 0, 96: 1, 112: 34603009, 128: 33555456, 144: 1048577, 160: 33554433, 176: 34604032, 192: 34603008, 208: 1025, 224: 1049600, 240: 33554432, 8: 34603009, 24: 0, 40: 33555457, 56: 34604032, 72: 1048576, 88: 33554433, 104: 33554432, 120: 1025, 136: 1049601, 152: 33555456, 168: 34603008, 184: 1048577, 200: 1024, 216: 34604033, 232: 1, 248: 1049600, 256: 33554432, 272: 1048576, 288: 33555457, 304: 34603009, 320: 1048577, 336: 33555456, 352: 34604032, 368: 1049601, 384: 1025, 400: 34604033, 416: 1049600, 432: 1, 448: 0, 464: 34603008, 480: 33554433, 496: 1024, 264: 1049600, 280: 33555457, 296: 34603009, 312: 1, 328: 33554432, 344: 1048576, 360: 1025, 376: 34604032, 392: 33554433, 408: 34603008, 424: 0, 440: 34604033, 456: 1049601, 472: 1024, 488: 33555456, 504: 1048577 }, { 0: 134219808, 1: 131072, 2: 134217728, 3: 32, 4: 131104, 5: 134350880, 6: 134350848, 7: 2048, 8: 134348800, 9: 134219776, 10: 133120, 11: 134348832, 12: 2080, 13: 0, 14: 134217760, 15: 133152, 2147483648: 2048, 2147483649: 134350880, 2147483650: 134219808, 2147483651: 134217728, 2147483652: 134348800, 2147483653: 133120, 2147483654: 133152, 2147483655: 32, 2147483656: 134217760, 2147483657: 2080, 2147483658: 131104, 2147483659: 134350848, 2147483660: 0, 2147483661: 134348832, 2147483662: 134219776, 2147483663: 131072, 16: 133152, 17: 134350848, 18: 32, 19: 2048, 20: 134219776, 21: 134217760, 22: 134348832, 23: 131072, 24: 0, 25: 131104, 26: 134348800, 27: 134219808, 28: 134350880, 29: 133120, 30: 2080, 31: 134217728, 2147483664: 131072, 2147483665: 2048, 2147483666: 134348832, 2147483667: 133152, 2147483668: 32, 2147483669: 134348800, 2147483670: 134217728, 2147483671: 134219808, 2147483672: 134350880, 2147483673: 134217760, 2147483674: 134219776, 2147483675: 0, 2147483676: 133120, 2147483677: 2080, 2147483678: 131104, 2147483679: 134350848 }],
            p = [4160749569, 528482304, 33030144, 2064384, 129024, 8064, 504, 2147483679],
            h = s.DES = a.extend({ _doReset: function () {
            for (var t = this._key.words, e = [], n = 0; n < 56; n++) {
              var r = c[n] - 1;e[n] = t[r >>> 5] >>> 31 - r % 32 & 1;
            }for (var o = this._subKeys = [], i = 0; i < 16; i++) {
              for (var a = o[i] = [], s = f[i], n = 0; n < 24; n++) a[n / 6 | 0] |= e[(u[n] - 1 + s) % 28] << 31 - n % 6, a[4 + (n / 6 | 0)] |= e[28 + (u[n + 24] - 1 + s) % 28] << 31 - n % 6;a[0] = a[0] << 1 | a[0] >>> 31;for (n = 1; n < 7; n++) a[n] = a[n] >>> 4 * (n - 1) + 3;a[7] = a[7] << 5 | a[7] >>> 27;
            }for (var l = this._invSubKeys = [], n = 0; n < 16; n++) l[n] = o[15 - n];
          }, encryptBlock: function (t, e) {
            this._doCryptBlock(t, e, this._subKeys);
          }, decryptBlock: function (t, e) {
            this._doCryptBlock(t, e, this._invSubKeys);
          }, _doCryptBlock: function (t, r, o) {
            this._lBlock = t[r], this._rBlock = t[r + 1], e.call(this, 4, 252645135), e.call(this, 16, 65535), n.call(this, 2, 858993459), n.call(this, 8, 16711935), e.call(this, 1, 1431655765);for (var i = 0; i < 16; i++) {
              for (var a = o[i], s = this._lBlock, c = this._rBlock, u = 0, f = 0; f < 8; f++) u |= l[f][((c ^ a[f]) & p[f]) >>> 0];this._lBlock = c, this._rBlock = s ^ u;
            }var h = this._lBlock;this._lBlock = this._rBlock, this._rBlock = h, e.call(this, 1, 1431655765), n.call(this, 8, 16711935), n.call(this, 2, 858993459), e.call(this, 16, 65535), e.call(this, 4, 252645135), t[r] = this._lBlock, t[r + 1] = this._rBlock;
          }, keySize: 2, ivSize: 2, blockSize: 2 });r.DES = a._createHelper(h);var d = s.TripleDES = a.extend({ _doReset: function () {
            var t = this._key.words;this._des1 = h.createEncryptor(i.create(t.slice(0, 2))), this._des2 = h.createEncryptor(i.create(t.slice(2, 4))), this._des3 = h.createEncryptor(i.create(t.slice(4, 6)));
          }, encryptBlock: function (t, e) {
            this._des1.encryptBlock(t, e), this._des2.decryptBlock(t, e), this._des3.encryptBlock(t, e);
          }, decryptBlock: function (t, e) {
            this._des3.decryptBlock(t, e), this._des2.encryptBlock(t, e), this._des1.decryptBlock(t, e);
          }, keySize: 6, ivSize: 2, blockSize: 2 });r.TripleDES = a._createHelper(d);
      }(), t.TripleDES;
    });
  }, { "./cipher-core": 51, "./core": 52, "./enc-base64": 53, "./evpkdf": 55, "./md5": 60 }], 83: [function (t, e, n) {
    !function (r, o) {
      "object" == typeof n ? e.exports = n = o(t("./core")) : "function" == typeof define && define.amd ? define(["./core"], o) : o(r.CryptoJS);
    }(this, function (t) {
      return function (e) {
        var n = t,
            r = n.lib,
            o = r.Base,
            i = r.WordArray,
            a = n.x64 = {};a.Word = o.extend({ init: function (t, e) {
            this.high = t, this.low = e;
          } }), a.WordArray = o.extend({ init: function (t, e) {
            t = this.words = t || [], this.sigBytes = void 0 != e ? e : 8 * t.length;
          }, toX32: function () {
            for (var t = this.words, e = t.length, n = [], r = 0; r < e; r++) {
              var o = t[r];n.push(o.high), n.push(o.low);
            }return i.create(n, this.sigBytes);
          }, clone: function () {
            for (var t = o.clone.call(this), e = t.words = this.words.slice(0), n = e.length, r = 0; r < n; r++) e[r] = e[r].clone();return t;
          } });
      }(), t;
    });
  }, { "./core": 52 }], 84: [function (t, e, n) {
    !function (t) {
      function r(t) {
        for (var e, n, r = [], o = 0, i = t.length; o < i;) (e = t.charCodeAt(o++)) >= 55296 && e <= 56319 && o < i ? 56320 == (64512 & (n = t.charCodeAt(o++))) ? r.push(((1023 & e) << 10) + (1023 & n) + 65536) : (r.push(e), o--) : r.push(e);return r;
      }function o(t) {
        for (var e, n = t.length, r = -1, o = ""; ++r < n;) (e = t[r]) > 65535 && (o += y((e -= 65536) >>> 10 & 1023 | 55296), e = 56320 | 1023 & e), o += y(e);return o;
      }function i(t) {
        if (t >= 55296 && t <= 57343) throw Error("Lone surrogate U+" + t.toString(16).toUpperCase() + " is not a scalar value");
      }function a(t, e) {
        return y(t >> e & 63 | 128);
      }function s(t) {
        if (0 == (4294967168 & t)) return y(t);var e = "";return 0 == (4294965248 & t) ? e = y(t >> 6 & 31 | 192) : 0 == (4294901760 & t) ? (i(t), e = y(t >> 12 & 15 | 224), e += a(t, 6)) : 0 == (4292870144 & t) && (e = y(t >> 18 & 7 | 240), e += a(t, 12), e += a(t, 6)), e += y(63 & t | 128);
      }function c() {
        if (m >= d) throw Error("Invalid byte index");var t = 255 & h[m];if (m++, 128 == (192 & t)) return 63 & t;throw Error("Invalid continuation byte");
      }function u() {
        var t, e, n, r, o;if (m > d) throw Error("Invalid byte index");if (m == d) return !1;if (t = 255 & h[m], m++, 0 == (128 & t)) return t;if (192 == (224 & t)) {
          if (e = c(), (o = (31 & t) << 6 | e) >= 128) return o;throw Error("Invalid continuation byte");
        }if (224 == (240 & t)) {
          if (e = c(), n = c(), (o = (15 & t) << 12 | e << 6 | n) >= 2048) return i(o), o;throw Error("Invalid continuation byte");
        }if (240 == (248 & t) && (e = c(), n = c(), r = c(), (o = (7 & t) << 18 | e << 12 | n << 6 | r) >= 65536 && o <= 1114111)) return o;throw Error("Invalid UTF-8 detected");
      }var f = "object" == typeof n && n,
          l = "object" == typeof e && e && e.exports == f && e,
          p = "object" == typeof global && global;p.global !== p && p.window !== p || (t = p);var h,
          d,
          m,
          y = String.fromCharCode,
          g = { version: "2.1.2", encode: function (t) {
          for (var e = r(t), n = e.length, o = -1, i = ""; ++o < n;) i += s(e[o]);return i;
        }, decode: function (t) {
          h = r(t), d = h.length, m = 0;for (var e, n = []; !1 !== (e = u());) n.push(e);return o(n);
        } };if ("function" == typeof define && "object" == typeof define.amd && define.amd) define(function () {
        return g;
      });else if (f && !f.nodeType) {
        if (l) l.exports = g;else {
          var v = {}.hasOwnProperty;for (var b in g) v.call(g, b) && (f[b] = g[b]);
        }
      } else t.utf8 = g;
    }(this);
  }, {}], 85: [function (t, e, n) {
    e.exports = XMLHttpRequest;
  }, {}], "bignumber.js": [function (t, e, n) {
    !function (t) {
      "use strict";
      function n(t) {
        function e(t, n) {
          var r,
              o,
              i,
              a,
              s,
              c,
              u = this;if (!(u instanceof e)) return U && I(26, "constructor call without new", t), new e(t, n);if (null != n && W(n, 2, 64, D, "base")) {
            if (n |= 0, c = t + "", 10 == n) return u = new e(t instanceof e ? t : c), N(u, M + u.e + 1, H);if ((a = "number" == typeof t) && 0 * t != 0 || !new RegExp("^-?" + (r = "[" + b.slice(0, n) + "]+") + "(?:\\." + r + ")?$", n < 37 ? "i" : "").test(c)) return P(u, c, a, n);a ? (u.s = 1 / t < 0 ? (c = c.slice(1), -1) : 1, U && c.replace(/^0\.0*|\./, "").length > 15 && I(D, v, t), a = !1) : u.s = 45 === c.charCodeAt(0) ? (c = c.slice(1), -1) : 1, c = p(c, 10, n, u.s);
          } else {
            if (t instanceof e) return u.s = t.s, u.e = t.e, u.c = (t = t.c) ? t.slice() : t, void (D = 0);if ((a = "number" == typeof t) && 0 * t == 0) {
              if (u.s = 1 / t < 0 ? (t = -t, -1) : 1, t === ~~t) {
                for (o = 0, i = t; i >= 10; i /= 10, o++);return u.e = o, u.c = [t], void (D = 0);
              }c = t + "";
            } else {
              if (!h.test(c = t + "")) return P(u, c, a);u.s = 45 === c.charCodeAt(0) ? (c = c.slice(1), -1) : 1;
            }
          }for ((o = c.indexOf(".")) > -1 && (c = c.replace(".", "")), (i = c.search(/e/i)) > 0 ? (o < 0 && (o = i), o += +c.slice(i + 1), c = c.substring(0, i)) : o < 0 && (o = c.length), i = 0; 48 === c.charCodeAt(i); i++);for (s = c.length; 48 === c.charCodeAt(--s););if (c = c.slice(i, s + 1)) {
            if (s = c.length, a && U && s > 15 && (t > x || t !== m(t)) && I(D, v, u.s * t), (o = o - i - 1) > L) u.c = u.e = null;else if (o < z) u.c = [u.e = 0];else {
              if (u.e = o, u.c = [], i = (o + 1) % w, o < 0 && (i += w), i < s) {
                for (i && u.c.push(+c.slice(0, i)), s -= w; i < s;) u.c.push(+c.slice(i, i += w));c = c.slice(i), i = w - c.length;
              } else i -= s;for (; i--; c += "0");u.c.push(+c);
            }
          } else u.c = [u.e = 0];D = 0;
        }function p(t, n, r, i) {
          var a,
              s,
              u,
              l,
              p,
              h,
              d,
              m = t.indexOf("."),
              y = M,
              g = H;for (r < 37 && (t = t.toLowerCase()), m >= 0 && (u = G, G = 0, t = t.replace(".", ""), p = (d = new e(r)).pow(t.length - m), G = u, d.c = c(f(o(p.c), p.e), 10, n), d.e = d.c.length), s = u = (h = c(t, r, n)).length; 0 == h[--u]; h.pop());if (!h[0]) return "0";if (m < 0 ? --s : (p.c = h, p.e = s, p.s = i, h = (p = T(p, d, y, g, n)).c, l = p.r, s = p.e), a = s + y + 1, m = h[a], u = n / 2, l = l || a < 0 || null != h[a + 1], l = g < 4 ? (null != m || l) && (0 == g || g == (p.s < 0 ? 3 : 2)) : m > u || m == u && (4 == g || l || 6 == g && 1 & h[a - 1] || g == (p.s < 0 ? 8 : 7)), a < 1 || !h[0]) t = l ? f("1", -y) : "0";else {
            if (h.length = a, l) for (--n; ++h[--a] > n;) h[a] = 0, a || (++s, h.unshift(1));for (u = h.length; !h[--u];);for (m = 0, t = ""; m <= u; t += b.charAt(h[m++]));t = f(t, s);
          }return t;
        }function C(t, n, r, i) {
          var a, s, c, l, p;if (r = null != r && W(r, 0, 8, i, g) ? 0 | r : H, !t.c) return t.toString();if (a = t.c[0], c = t.e, null == n) p = o(t.c), p = 19 == i || 24 == i && c <= j ? u(p, c) : f(p, c);else if (t = N(new e(t), n, r), s = t.e, p = o(t.c), l = p.length, 19 == i || 24 == i && (n <= s || s <= j)) {
            for (; l < n; p += "0", l++);p = u(p, s);
          } else if (n -= c, p = f(p, s), s + 1 > l) {
            if (--n > 0) for (p += "."; n--; p += "0");
          } else if ((n += s - l) > 0) for (s + 1 == l && (p += "."); n--; p += "0");return t.s < 0 && a ? "-" + p : p;
        }function A(t, n) {
          var r,
              o,
              i = 0;for (s(t[0]) && (t = t[0]), r = new e(t[0]); ++i < t.length;) {
            if (!(o = new e(t[i])).s) {
              r = o;break;
            }n.call(r, o) && (r = o);
          }return r;
        }function F(t, e, n, r, o) {
          return (t < e || t > n || t != l(t)) && I(r, (o || "decimal places") + (t < e || t > n ? " out of range" : " not an integer"), t), !0;
        }function O(t, e, n) {
          for (var r = 1, o = e.length; !e[--o]; e.pop());for (o = e[0]; o >= 10; o /= 10, r++);return (n = r + n * w - 1) > L ? t.c = t.e = null : n < z ? t.c = [t.e = 0] : (t.e = n, t.c = e), t;
        }function I(t, e, n) {
          var r = new Error(["new BigNumber", "cmp", "config", "div", "divToInt", "eq", "gt", "gte", "lt", "lte", "minus", "mod", "plus", "precision", "random", "round", "shift", "times", "toDigits", "toExponential", "toFixed", "toFormat", "toFraction", "pow", "toPrecision", "toString", "BigNumber"][t] + "() " + e + ": " + n);throw r.name = "BigNumber Error", D = 0, r;
        }function N(t, e, n, r) {
          var o,
              i,
              a,
              s,
              c,
              u,
              f,
              l = t.c,
              p = k;if (l) {
            t: {
              for (o = 1, s = l[0]; s >= 10; s /= 10, o++);if ((i = e - o) < 0) i += w, a = e, f = (c = l[u = 0]) / p[o - a - 1] % 10 | 0;else if ((u = d((i + 1) / w)) >= l.length) {
                if (!r) break t;for (; l.length <= u; l.push(0));c = f = 0, o = 1, a = (i %= w) - w + 1;
              } else {
                for (c = s = l[u], o = 1; s >= 10; s /= 10, o++);f = (a = (i %= w) - w + o) < 0 ? 0 : c / p[o - a - 1] % 10 | 0;
              }if (r = r || e < 0 || null != l[u + 1] || (a < 0 ? c : c % p[o - a - 1]), r = n < 4 ? (f || r) && (0 == n || n == (t.s < 0 ? 3 : 2)) : f > 5 || 5 == f && (4 == n || r || 6 == n && (i > 0 ? a > 0 ? c / p[o - a] : 0 : l[u - 1]) % 10 & 1 || n == (t.s < 0 ? 8 : 7)), e < 1 || !l[0]) return l.length = 0, r ? (e -= t.e + 1, l[0] = p[(w - e % w) % w], t.e = -e || 0) : l[0] = t.e = 0, t;if (0 == i ? (l.length = u, s = 1, u--) : (l.length = u + 1, s = p[w - i], l[u] = a > 0 ? m(c / p[o - a] % p[a]) * s : 0), r) for (;;) {
                if (0 == u) {
                  for (i = 1, a = l[0]; a >= 10; a /= 10, i++);for (a = l[0] += s, s = 1; a >= 10; a /= 10, s++);i != s && (t.e++, l[0] == _ && (l[0] = 1));break;
                }if (l[u] += s, l[u] != _) break;l[u--] = 0, s = 1;
              }for (i = l.length; 0 === l[--i]; l.pop());
            }t.e > L ? t.c = t.e = null : t.e < z && (t.c = [t.e = 0]);
          }return t;
        }var T,
            P,
            D = 0,
            R = e.prototype,
            E = new e(1),
            M = 20,
            H = 4,
            j = -7,
            q = 21,
            z = -1e7,
            L = 1e7,
            U = !0,
            W = F,
            J = !1,
            K = 1,
            G = 0,
            X = { decimalSeparator: ".", groupSeparator: ",", groupSize: 3, secondaryGroupSize: 0, fractionGroupSeparator: " ", fractionGroupSize: 0 };return e.another = n, e.ROUND_UP = 0, e.ROUND_DOWN = 1, e.ROUND_CEIL = 2, e.ROUND_FLOOR = 3, e.ROUND_HALF_UP = 4, e.ROUND_HALF_DOWN = 5, e.ROUND_HALF_EVEN = 6, e.ROUND_HALF_CEIL = 7, e.ROUND_HALF_FLOOR = 8, e.EUCLID = 9, e.config = e.set = function () {
          var t,
              e,
              n = 0,
              r = {},
              o = arguments,
              i = o[0],
              c = i && "object" == typeof i ? function () {
            if (i.hasOwnProperty(e)) return null != (t = i[e]);
          } : function () {
            if (o.length > n) return null != (t = o[n++]);
          };return c(e = "DECIMAL_PLACES") && W(t, 0, S, 2, e) && (M = 0 | t), r[e] = M, c(e = "ROUNDING_MODE") && W(t, 0, 8, 2, e) && (H = 0 | t), r[e] = H, c(e = "EXPONENTIAL_AT") && (s(t) ? W(t[0], -S, 0, 2, e) && W(t[1], 0, S, 2, e) && (j = 0 | t[0], q = 0 | t[1]) : W(t, -S, S, 2, e) && (j = -(q = 0 | (t < 0 ? -t : t)))), r[e] = [j, q], c(e = "RANGE") && (s(t) ? W(t[0], -S, -1, 2, e) && W(t[1], 1, S, 2, e) && (z = 0 | t[0], L = 0 | t[1]) : W(t, -S, S, 2, e) && (0 | t ? z = -(L = 0 | (t < 0 ? -t : t)) : U && I(2, e + " cannot be zero", t))), r[e] = [z, L], c(e = "ERRORS") && (t === !!t || 1 === t || 0 === t ? (D = 0, W = (U = !!t) ? F : a) : U && I(2, e + y, t)), r[e] = U, c(e = "CRYPTO") && (!0 === t || !1 === t || 1 === t || 0 === t ? t ? !(t = "undefined" == typeof crypto) && crypto && (crypto.getRandomValues || crypto.randomBytes) ? J = !0 : U ? I(2, "crypto unavailable", t ? void 0 : crypto) : J = !1 : J = !1 : U && I(2, e + y, t)), r[e] = J, c(e = "MODULO_MODE") && W(t, 0, 9, 2, e) && (K = 0 | t), r[e] = K, c(e = "POW_PRECISION") && W(t, 0, S, 2, e) && (G = 0 | t), r[e] = G, c(e = "FORMAT") && ("object" == typeof t ? X = t : U && I(2, e + " not an object", t)), r[e] = X, r;
        }, e.max = function () {
          return A(arguments, R.lt);
        }, e.min = function () {
          return A(arguments, R.gt);
        }, e.random = function () {
          var t = 9007199254740992 * Math.random() & 2097151 ? function () {
            return m(9007199254740992 * Math.random());
          } : function () {
            return 8388608 * (1073741824 * Math.random() | 0) + (8388608 * Math.random() | 0);
          };return function (n) {
            var r,
                o,
                i,
                a,
                s,
                c = 0,
                u = [],
                f = new e(E);if (n = null != n && W(n, 0, S, 14) ? 0 | n : M, a = d(n / w), J) if (crypto.getRandomValues) {
              for (r = crypto.getRandomValues(new Uint32Array(a *= 2)); c < a;) (s = 131072 * r[c] + (r[c + 1] >>> 11)) >= 9e15 ? (o = crypto.getRandomValues(new Uint32Array(2)), r[c] = o[0], r[c + 1] = o[1]) : (u.push(s % 1e14), c += 2);c = a / 2;
            } else if (crypto.randomBytes) {
              for (r = crypto.randomBytes(a *= 7); c < a;) (s = 281474976710656 * (31 & r[c]) + 1099511627776 * r[c + 1] + 4294967296 * r[c + 2] + 16777216 * r[c + 3] + (r[c + 4] << 16) + (r[c + 5] << 8) + r[c + 6]) >= 9e15 ? crypto.randomBytes(7).copy(r, c) : (u.push(s % 1e14), c += 7);c = a / 7;
            } else J = !1, U && I(14, "crypto unavailable", crypto);if (!J) for (; c < a;) (s = t()) < 9e15 && (u[c++] = s % 1e14);for (a = u[--c], n %= w, a && n && (s = k[w - n], u[c] = m(a / s) * s); 0 === u[c]; u.pop(), c--);if (c < 0) u = [i = 0];else {
              for (i = -1; 0 === u[0]; u.shift(), i -= w);for (c = 1, s = u[0]; s >= 10; s /= 10, c++);c < w && (i -= w - c);
            }return f.e = i, f.c = u, f;
          };
        }(), T = function () {
          function t(t, e, n) {
            var r,
                o,
                i,
                a,
                s = 0,
                c = t.length,
                u = e % B,
                f = e / B | 0;for (t = t.slice(); c--;) s = ((o = u * (i = t[c] % B) + (r = f * i + (a = t[c] / B | 0) * u) % B * B + s) / n | 0) + (r / B | 0) + f * a, t[c] = o % n;return s && t.unshift(s), t;
          }function n(t, e, n, r) {
            var o, i;if (n != r) i = n > r ? 1 : -1;else for (o = i = 0; o < n; o++) if (t[o] != e[o]) {
              i = t[o] > e[o] ? 1 : -1;break;
            }return i;
          }function o(t, e, n, r) {
            for (var o = 0; n--;) t[n] -= o, o = t[n] < e[n] ? 1 : 0, t[n] = o * r + t[n] - e[n];for (; !t[0] && t.length > 1; t.shift());
          }return function (i, a, s, c, u) {
            var f,
                l,
                p,
                h,
                d,
                y,
                g,
                v,
                b,
                x,
                k,
                B,
                S,
                C,
                A,
                F,
                O,
                I = i.s == a.s ? 1 : -1,
                T = i.c,
                P = a.c;if (!(T && T[0] && P && P[0])) return new e(i.s && a.s && (T ? !P || T[0] != P[0] : P) ? T && 0 == T[0] || !P ? 0 * I : I / 0 : NaN);for (b = (v = new e(I)).c = [], I = s + (l = i.e - a.e) + 1, u || (u = _, l = r(i.e / w) - r(a.e / w), I = I / w | 0), p = 0; P[p] == (T[p] || 0); p++);if (P[p] > (T[p] || 0) && l--, I < 0) b.push(1), h = !0;else {
              for (C = T.length, F = P.length, p = 0, I += 2, (d = m(u / (P[0] + 1))) > 1 && (P = t(P, d, u), T = t(T, d, u), F = P.length, C = T.length), S = F, k = (x = T.slice(0, F)).length; k < F; x[k++] = 0);(O = P.slice()).unshift(0), A = P[0], P[1] >= u / 2 && A++;do {
                if (d = 0, (f = n(P, x, F, k)) < 0) {
                  if (B = x[0], F != k && (B = B * u + (x[1] || 0)), (d = m(B / A)) > 1) for (d >= u && (d = u - 1), g = (y = t(P, d, u)).length, k = x.length; 1 == n(y, x, g, k);) d--, o(y, F < g ? O : P, g, u), g = y.length, f = 1;else 0 == d && (f = d = 1), g = (y = P.slice()).length;if (g < k && y.unshift(0), o(x, y, k, u), k = x.length, -1 == f) for (; n(P, x, F, k) < 1;) d++, o(x, F < k ? O : P, k, u), k = x.length;
                } else 0 === f && (d++, x = [0]);b[p++] = d, x[0] ? x[k++] = T[S] || 0 : (x = [T[S]], k = 1);
              } while ((S++ < C || null != x[0]) && I--);h = null != x[0], b[0] || b.shift();
            }if (u == _) {
              for (p = 1, I = b[0]; I >= 10; I /= 10, p++);N(v, s + (v.e = p + l * w - 1) + 1, c, h);
            } else v.e = l, v.r = +h;return v;
          };
        }(), P = function () {
          var t = /^(-?)0([xbo])(?=\w[\w.]*$)/i,
              n = /^([^.]+)\.$/,
              r = /^\.([^.]+)$/,
              o = /^-?(Infinity|NaN)$/,
              i = /^\s*\+(?=[\w.])|^\s+|\s+$/g;return function (a, s, c, u) {
            var f,
                l = c ? s : s.replace(i, "");if (o.test(l)) a.s = isNaN(l) ? null : l < 0 ? -1 : 1;else {
              if (!c && (l = l.replace(t, function (t, e, n) {
                return f = "x" == (n = n.toLowerCase()) ? 16 : "b" == n ? 2 : 8, u && u != f ? t : e;
              }), u && (f = u, l = l.replace(n, "$1").replace(r, "0.$1")), s != l)) return new e(l, f);U && I(D, "not a" + (u ? " base " + u : "") + " number", s), a.s = null;
            }a.c = a.e = null, D = 0;
          };
        }(), R.absoluteValue = R.abs = function () {
          var t = new e(this);return t.s < 0 && (t.s = 1), t;
        }, R.ceil = function () {
          return N(new e(this), this.e + 1, 2);
        }, R.comparedTo = R.cmp = function (t, n) {
          return D = 1, i(this, new e(t, n));
        }, R.decimalPlaces = R.dp = function () {
          var t,
              e,
              n = this.c;if (!n) return null;if (t = ((e = n.length - 1) - r(this.e / w)) * w, e = n[e]) for (; e % 10 == 0; e /= 10, t--);return t < 0 && (t = 0), t;
        }, R.dividedBy = R.div = function (t, n) {
          return D = 3, T(this, new e(t, n), M, H);
        }, R.dividedToIntegerBy = R.divToInt = function (t, n) {
          return D = 4, T(this, new e(t, n), 0, 1);
        }, R.equals = R.eq = function (t, n) {
          return D = 5, 0 === i(this, new e(t, n));
        }, R.floor = function () {
          return N(new e(this), this.e + 1, 3);
        }, R.greaterThan = R.gt = function (t, n) {
          return D = 6, i(this, new e(t, n)) > 0;
        }, R.greaterThanOrEqualTo = R.gte = function (t, n) {
          return D = 7, 1 === (n = i(this, new e(t, n))) || 0 === n;
        }, R.isFinite = function () {
          return !!this.c;
        }, R.isInteger = R.isInt = function () {
          return !!this.c && r(this.e / w) > this.c.length - 2;
        }, R.isNaN = function () {
          return !this.s;
        }, R.isNegative = R.isNeg = function () {
          return this.s < 0;
        }, R.isZero = function () {
          return !!this.c && 0 == this.c[0];
        }, R.lessThan = R.lt = function (t, n) {
          return D = 8, i(this, new e(t, n)) < 0;
        }, R.lessThanOrEqualTo = R.lte = function (t, n) {
          return D = 9, -1 === (n = i(this, new e(t, n))) || 0 === n;
        }, R.minus = R.sub = function (t, n) {
          var o,
              i,
              a,
              s,
              c = this,
              u = c.s;if (D = 10, t = new e(t, n), n = t.s, !u || !n) return new e(NaN);if (u != n) return t.s = -n, c.plus(t);var f = c.e / w,
              l = t.e / w,
              p = c.c,
              h = t.c;if (!f || !l) {
            if (!p || !h) return p ? (t.s = -n, t) : new e(h ? c : NaN);if (!p[0] || !h[0]) return h[0] ? (t.s = -n, t) : new e(p[0] ? c : 3 == H ? -0 : 0);
          }if (f = r(f), l = r(l), p = p.slice(), u = f - l) {
            for ((s = u < 0) ? (u = -u, a = p) : (l = f, a = h), a.reverse(), n = u; n--; a.push(0));a.reverse();
          } else for (i = (s = (u = p.length) < (n = h.length)) ? u : n, u = n = 0; n < i; n++) if (p[n] != h[n]) {
            s = p[n] < h[n];break;
          }if (s && (a = p, p = h, h = a, t.s = -t.s), (n = (i = h.length) - (o = p.length)) > 0) for (; n--; p[o++] = 0);for (n = _ - 1; i > u;) {
            if (p[--i] < h[i]) {
              for (o = i; o && !p[--o]; p[o] = n);--p[o], p[i] += _;
            }p[i] -= h[i];
          }for (; 0 == p[0]; p.shift(), --l);return p[0] ? O(t, p, l) : (t.s = 3 == H ? -1 : 1, t.c = [t.e = 0], t);
        }, R.modulo = R.mod = function (t, n) {
          var r,
              o,
              i = this;return D = 11, t = new e(t, n), !i.c || !t.s || t.c && !t.c[0] ? new e(NaN) : !t.c || i.c && !i.c[0] ? new e(i) : (9 == K ? (o = t.s, t.s = 1, r = T(i, t, 0, 3), t.s = o, r.s *= o) : r = T(i, t, 0, K), i.minus(r.times(t)));
        }, R.negated = R.neg = function () {
          var t = new e(this);return t.s = -t.s || null, t;
        }, R.plus = R.add = function (t, n) {
          var o,
              i = this,
              a = i.s;if (D = 12, t = new e(t, n), n = t.s, !a || !n) return new e(NaN);if (a != n) return t.s = -n, i.minus(t);var s = i.e / w,
              c = t.e / w,
              u = i.c,
              f = t.c;if (!s || !c) {
            if (!u || !f) return new e(a / 0);if (!u[0] || !f[0]) return f[0] ? t : new e(u[0] ? i : 0 * a);
          }if (s = r(s), c = r(c), u = u.slice(), a = s - c) {
            for (a > 0 ? (c = s, o = f) : (a = -a, o = u), o.reverse(); a--; o.push(0));o.reverse();
          }for ((a = u.length) - (n = f.length) < 0 && (o = f, f = u, u = o, n = a), a = 0; n;) a = (u[--n] = u[n] + f[n] + a) / _ | 0, u[n] = _ === u[n] ? 0 : u[n] % _;return a && (u.unshift(a), ++c), O(t, u, c);
        }, R.precision = R.sd = function (t) {
          var e,
              n,
              r = this,
              o = r.c;if (null != t && t !== !!t && 1 !== t && 0 !== t && (U && I(13, "argument" + y, t), t != !!t && (t = null)), !o) return null;if (n = o.length - 1, e = n * w + 1, n = o[n]) {
            for (; n % 10 == 0; n /= 10, e--);for (n = o[0]; n >= 10; n /= 10, e++);
          }return t && r.e + 1 > e && (e = r.e + 1), e;
        }, R.round = function (t, n) {
          var r = new e(this);return (null == t || W(t, 0, S, 15)) && N(r, ~~t + this.e + 1, null != n && W(n, 0, 8, 15, g) ? 0 | n : H), r;
        }, R.shift = function (t) {
          var n = this;return W(t, -x, x, 16, "argument") ? n.times("1e" + l(t)) : new e(n.c && n.c[0] && (t < -x || t > x) ? n.s * (t < 0 ? 0 : 1 / 0) : n);
        }, R.squareRoot = R.sqrt = function () {
          var t,
              n,
              i,
              a,
              s,
              c = this,
              u = c.c,
              f = c.s,
              l = c.e,
              p = M + 4,
              h = new e("0.5");if (1 !== f || !u || !u[0]) return new e(!f || f < 0 && (!u || u[0]) ? NaN : u ? c : 1 / 0);if (0 == (f = Math.sqrt(+c)) || f == 1 / 0 ? (((n = o(u)).length + l) % 2 == 0 && (n += "0"), f = Math.sqrt(n), l = r((l + 1) / 2) - (l < 0 || l % 2), i = new e(n = f == 1 / 0 ? "1e" + l : (n = f.toExponential()).slice(0, n.indexOf("e") + 1) + l)) : i = new e(f + ""), i.c[0]) for ((f = (l = i.e) + p) < 3 && (f = 0);;) if (s = i, i = h.times(s.plus(T(c, s, p, 1))), o(s.c).slice(0, f) === (n = o(i.c)).slice(0, f)) {
            if (i.e < l && --f, "9999" != (n = n.slice(f - 3, f + 1)) && (a || "4999" != n)) {
              +n && (+n.slice(1) || "5" != n.charAt(0)) || (N(i, i.e + M + 2, 1), t = !i.times(i).eq(c));break;
            }if (!a && (N(s, s.e + M + 2, 0), s.times(s).eq(c))) {
              i = s;break;
            }p += 4, f += 4, a = 1;
          }return N(i, i.e + M + 1, H, t);
        }, R.times = R.mul = function (t, n) {
          var o,
              i,
              a,
              s,
              c,
              u,
              f,
              l,
              p,
              h,
              d,
              m,
              y,
              g,
              v,
              b = this,
              x = b.c,
              k = (D = 17, t = new e(t, n)).c;if (!(x && k && x[0] && k[0])) return !b.s || !t.s || x && !x[0] && !k || k && !k[0] && !x ? t.c = t.e = t.s = null : (t.s *= b.s, x && k ? (t.c = [0], t.e = 0) : t.c = t.e = null), t;for (i = r(b.e / w) + r(t.e / w), t.s *= b.s, (f = x.length) < (h = k.length) && (y = x, x = k, k = y, a = f, f = h, h = a), a = f + h, y = []; a--; y.push(0));for (g = _, v = B, a = h; --a >= 0;) {
            for (o = 0, d = k[a] % v, m = k[a] / v | 0, s = a + (c = f); s > a;) o = ((l = d * (l = x[--c] % v) + (u = m * l + (p = x[c] / v | 0) * d) % v * v + y[s] + o) / g | 0) + (u / v | 0) + m * p, y[s--] = l % g;y[s] = o;
          }return o ? ++i : y.shift(), O(t, y, i);
        }, R.toDigits = function (t, n) {
          var r = new e(this);return t = null != t && W(t, 1, S, 18, "precision") ? 0 | t : null, n = null != n && W(n, 0, 8, 18, g) ? 0 | n : H, t ? N(r, t, n) : r;
        }, R.toExponential = function (t, e) {
          return C(this, null != t && W(t, 0, S, 19) ? 1 + ~~t : null, e, 19);
        }, R.toFixed = function (t, e) {
          return C(this, null != t && W(t, 0, S, 20) ? ~~t + this.e + 1 : null, e, 20);
        }, R.toFormat = function (t, e) {
          var n = C(this, null != t && W(t, 0, S, 21) ? ~~t + this.e + 1 : null, e, 21);if (this.c) {
            var r,
                o = n.split("."),
                i = +X.groupSize,
                a = +X.secondaryGroupSize,
                s = X.groupSeparator,
                c = o[0],
                u = o[1],
                f = this.s < 0,
                l = f ? c.slice(1) : c,
                p = l.length;if (a && (r = i, i = a, a = r, p -= r), i > 0 && p > 0) {
              for (r = p % i || i, c = l.substr(0, r); r < p; r += i) c += s + l.substr(r, i);a > 0 && (c += s + l.slice(r)), f && (c = "-" + c);
            }n = u ? c + X.decimalSeparator + ((a = +X.fractionGroupSize) ? u.replace(new RegExp("\\d{" + a + "}\\B", "g"), "$&" + X.fractionGroupSeparator) : u) : c;
          }return n;
        }, R.toFraction = function (t) {
          var n,
              r,
              i,
              a,
              s,
              c,
              u,
              f,
              l,
              p = U,
              h = this,
              d = h.c,
              m = new e(E),
              y = r = new e(E),
              g = u = new e(E);if (null != t && (U = !1, c = new e(t), U = p, (p = c.isInt()) && !c.lt(E) || (U && I(22, "max denominator " + (p ? "out of range" : "not an integer"), t), t = !p && c.c && N(c, c.e + 1, 1).gte(E) ? c : null)), !d) return h.toString();for (l = o(d), a = m.e = l.length - h.e - 1, m.c[0] = k[(s = a % w) < 0 ? w + s : s], t = !t || c.cmp(m) > 0 ? a > 0 ? m : y : c, s = L, L = 1 / 0, c = new e(l), u.c[0] = 0; f = T(c, m, 0, 1), 1 != (i = r.plus(f.times(g))).cmp(t);) r = g, g = i, y = u.plus(f.times(i = y)), u = i, m = c.minus(f.times(i = m)), c = i;return i = T(t.minus(r), g, 0, 1), u = u.plus(i.times(y)), r = r.plus(i.times(g)), u.s = y.s = h.s, a *= 2, n = T(y, g, a, H).minus(h).abs().cmp(T(u, r, a, H).minus(h).abs()) < 1 ? [y.toString(), g.toString()] : [u.toString(), r.toString()], L = s, n;
        }, R.toNumber = function () {
          return +this;
        }, R.toPower = R.pow = function (t, n) {
          var r,
              o,
              i,
              a = m(t < 0 ? -t : +t),
              s = this;if (null != n && (D = 23, n = new e(n)), !W(t, -x, x, 23, "exponent") && (!isFinite(t) || a > x && (t /= 0) || parseFloat(t) != t && !(t = NaN)) || 0 == t) return r = Math.pow(+s, t), new e(n ? r % n : r);for (n ? t > 1 && s.gt(E) && s.isInt() && n.gt(E) && n.isInt() ? s = s.mod(n) : (i = n, n = null) : G && (r = d(G / w + 2)), o = new e(E);;) {
            if (a % 2) {
              if (!(o = o.times(s)).c) break;r ? o.c.length > r && (o.c.length = r) : n && (o = o.mod(n));
            }if (!(a = m(a / 2))) break;s = s.times(s), r ? s.c && s.c.length > r && (s.c.length = r) : n && (s = s.mod(n));
          }return n ? o : (t < 0 && (o = E.div(o)), i ? o.mod(i) : r ? N(o, G, H) : o);
        }, R.toPrecision = function (t, e) {
          return C(this, null != t && W(t, 1, S, 24, "precision") ? 0 | t : null, e, 24);
        }, R.toString = function (t) {
          var e,
              n = this,
              r = n.s,
              i = n.e;return null === i ? r ? (e = "Infinity", r < 0 && (e = "-" + e)) : e = "NaN" : (e = o(n.c), e = null != t && W(t, 2, 64, 25, "base") ? p(f(e, i), 0 | t, 10, r) : i <= j || i >= q ? u(e, i) : f(e, i), r < 0 && n.c[0] && (e = "-" + e)), e;
        }, R.truncated = R.trunc = function () {
          return N(new e(this), this.e + 1, 1);
        }, R.valueOf = R.toJSON = function () {
          var t,
              e = this,
              n = e.e;return null === n ? e.toString() : (t = o(e.c), t = n <= j || n >= q ? u(t, n) : f(t, n), e.s < 0 ? "-" + t : t);
        }, R.isBigNumber = !0, null != t && e.config(t), e;
      }function r(t) {
        var e = 0 | t;return t > 0 || t === e ? e : e - 1;
      }function o(t) {
        for (var e, n, r = 1, o = t.length, i = t[0] + ""; r < o;) {
          for (e = t[r++] + "", n = w - e.length; n--; e = "0" + e);i += e;
        }for (o = i.length; 48 === i.charCodeAt(--o););return i.slice(0, o + 1 || 1);
      }function i(t, e) {
        var n,
            r,
            o = t.c,
            i = e.c,
            a = t.s,
            s = e.s,
            c = t.e,
            u = e.e;if (!a || !s) return null;if (n = o && !o[0], r = i && !i[0], n || r) return n ? r ? 0 : -s : a;if (a != s) return a;if (n = a < 0, r = c == u, !o || !i) return r ? 0 : !o ^ n ? 1 : -1;if (!r) return c > u ^ n ? 1 : -1;for (s = (c = o.length) < (u = i.length) ? c : u, a = 0; a < s; a++) if (o[a] != i[a]) return o[a] > i[a] ^ n ? 1 : -1;return c == u ? 0 : c > u ^ n ? 1 : -1;
      }function a(t, e, n) {
        return (t = l(t)) >= e && t <= n;
      }function s(t) {
        return "[object Array]" == Object.prototype.toString.call(t);
      }function c(t, e, n) {
        for (var r, o, i = [0], a = 0, s = t.length; a < s;) {
          for (o = i.length; o--; i[o] *= e);for (i[r = 0] += b.indexOf(t.charAt(a++)); r < i.length; r++) i[r] > n - 1 && (null == i[r + 1] && (i[r + 1] = 0), i[r + 1] += i[r] / n | 0, i[r] %= n);
        }return i.reverse();
      }function u(t, e) {
        return (t.length > 1 ? t.charAt(0) + "." + t.slice(1) : t) + (e < 0 ? "e" : "e+") + e;
      }function f(t, e) {
        var n, r;if (e < 0) {
          for (r = "0."; ++e; r += "0");t = r + t;
        } else if (n = t.length, ++e > n) {
          for (r = "0", e -= n; --e; r += "0");t += r;
        } else e < n && (t = t.slice(0, e) + "." + t.slice(e));return t;
      }function l(t) {
        return (t = parseFloat(t)) < 0 ? d(t) : m(t);
      }var p,
          h = /^-?(\d+(\.\d*)?|\.\d+)(e[+-]?\d+)?$/i,
          d = Math.ceil,
          m = Math.floor,
          y = " not a boolean or binary digit",
          g = "rounding mode",
          v = "number type has more than 15 significant digits",
          b = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$_",
          _ = 1e14,
          w = 14,
          x = 9007199254740991,
          k = [1, 10, 100, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13],
          B = 1e7,
          S = 1e9;(p = n()).default = p.BigNumber = p, "function" == typeof define && define.amd ? define(function () {
        return p;
      }) : void 0 !== e && e.exports ? e.exports = p : (t || (t = "undefined" != typeof self ? self : Function("return this")()), t.BigNumber = p);
    }(this);
  }, {}], web3: [function (t, e, n) {
    var r = t("./lib/web3");"undefined" != typeof window && void 0 === window.Web3 && (window.Web3 = r), e.exports = r;
  }, { "./lib/web3": 22 }] }, {}, ["web3"]);
