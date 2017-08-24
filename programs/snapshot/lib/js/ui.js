//minimal UI for now calls for vanilla js and some helpers for cleaner business logic

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
    elem = document.getElementById(current_step), elem ? elem.getElementsByClassName('status')[0].innerHTML = text : false;
  }

  const download_buttons = () => {
    display_text(`
      Export
      <a href="#" class="btn" onclick="download_snapshot()">Snapshot CSV</a>
      <a href="#" class="btn" onclick="download_rejects()">Rejects CSV</a>
      <a href="#" class="btn" onclick="download_reclaimable()">Reclaimable TX CSV</a>
      <a href="#" class="btn" onclick="download_reclaimed()">Successfully Reclaimed CSV</a>
      <a href="#" class="btn" onclick="download_logs()">Full Log</a>
    `)
  }

  if( typeof console === 'object') {
    try {
      if( typeof console.on === 'undefined')    console.on = () => { return CONSOLE_LOGGING = true }
      if( typeof console.off === 'undefined' )  console.off = () => { return CONSOLE_LOGGING = false }
    } catch(e) {}
  }

  //quick, ugly, dirty GUI >_<
  setInterval( () => {
    debug.refresh()

    // Complete
    if(debug.rates.percent_complete == 100) {
      display_text(`Snapshot <span>${debug.rates.percent_complete}%</span> Complete`)
      current_step = 'complete'
      download_buttons()
      // document.body.className += ' ' + "complete"
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

  }, 500)

})()
