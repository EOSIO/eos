//minimal UI for now calls for vanilla js and some helpers for cleaner business logic
let ui_refresh

(function(){

  let current_step 

  // Enable navigation prompt
  window.onbeforeunload = function() {
      return true;
  };

  // document.title updates
  const update_title_bar_progress = (stage = "") => {
    let title;
    switch( stage ){

      case 'distribute':
        title = `EOS | Snapshot ${debug.rates.percent_complete}% | ${debug.registry.accepted+debug.registry.rejected}/${debug.registry.total}`
      break
      case 'registry':
        title = `EOS | Building Registry | ${registrants.length}`
      break
      case 'reclaimer':
        title = `EOS | Reclaimable TXs | ${ debug.reclaimable.total }`
      break
      case 'complete':
        title = `EOS | Snapshot Complete`
      break
      default:
        title = `EOS | Welcome`
      break
    }
    document.title = title
  }

  const display_text = text => {
    let elem = document.getElementById(current_step)
    elem ? elem.getElementsByClassName('status')[0].innerHTML = text : false;
  }

  const download_buttons = () => {
    let buttons = `
      Export
      <div>
      <span class="output"><span>Snapshot &nbsp;</span><a href="#" class="btn" onclick="download_snapshot_csv()">CSV</a><a href="#" class="btn" onclick="download_snapshot_json()">JSON</a></span>
      <span class="output"><span>Rejects &nbsp;</span><a href="#" class="btn" onclick="download_rejects_csv()">CSV</a><a href="#" class="btn" onclick="download_rejects_json()">JSON</a></span>
      <span class="output"><span>Reclaimable TXs &nbsp;</span><a href="#" class="btn" onclick="download_reclaimable_csv()">CSV</a><a href="#" class="btn" onclick="download_reclaimable_json()">JSON</a></span>
      <span class="output"><span>Reclaimed TXs &nbsp;</span><a href="#" class="btn" onclick="download_reclaimed_csv()">CSV</a><a href="#" class="btn" onclick="download_reclaimed_json()">JSON</a></span>`
    buttons += (OUTPUT_LOGGING) ? `<span class="output"><span>Output Log &nbsp;</span><a href="#" class="btn" onclick="download_logs()">LOG</a></span>` : ""
    buttons += "</div>"
    display_text(buttons);
  }

  if( typeof console === 'object') {
    try {
      if( typeof console.on === 'undefined')    console.on = () => { return CONSOLE_LOGGING = true }
      if( typeof console.off === 'undefined' )  console.off = () => { return CONSOLE_LOGGING = false }
    } catch(e) {}
  }

  //quick, ugly, dirty GUI >_<
  ui_refresh = setInterval( () => {
    let disconnected = false, syncing = false

    if       ( !web3.isConnected() )  document.body.id = "disconnected", disconnected = true
    else if  ( !is_synced() )         document.body.id = "syncing", syncing = true
    else                              document.body.id = "ready", debug.refresh()

    if( disconnected ) {
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
      `
    }

    else if(syncing) {
      document.getElementsByClassName('disconnected')[0].innerHTML = `
        <p>It looks as though your node is still syncing, in order to run this script, your node will need to be synced up to block #${SS_LAST_BLOCK}, 
        which means you have ${SS_LAST_BLOCK-web3.eth.syncing.currentBlock} blocks to go. <em>Currently synced up to block ${web3.eth.syncing.currentBlock}</em>
        <p>Script will continue after your blockchain is synced.</p>
      `
    }

    // Complete
    else if(debug.rates.percent_complete == 100) {
      display_text(`Snapshot <span>${debug.rates.percent_complete}%</span> Complete`)
      current_step = 'complete'
      download_buttons()
      clearInterval(ui_refresh)
    }

    // Dist
    else if( get_registrants_accepted().length ) {
      current_step = "distribute"
      display_text(`Snapshot <span>${debug.rates.percent_complete}% <span>${debug.registry.accepted+debug.registry.rejected}/${debug.registry.total}</span></span>`)
    }

    // Reclaimables
    else if( debug.reclaimable.total ){
      current_step = "reclaimer"
      display_text(`Found <span>${ debug.reclaimable.total }</span> Reclaimable TXs`)
    }

    // Registry
    else if ( registrants.length ){
      current_step = "registry"
      display_text(`Found <span>${ registrants.length}</span> Registrants`)
      
    }

    // Page just loaded
    else {
      current_step = "welcome"
      display_text(`<a href="#" class="btn big" onclick="init()">Generate Snapshot<a/>`)
    }

    update_title_bar_progress(current_step)
    if(!document.body.className.includes(current_step)) document.body.className += ' ' + current_step;

  }, 1000)

})()
