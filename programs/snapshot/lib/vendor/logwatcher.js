// Hack to get at Register and Transfer logs
const LogWatcher = ( block_first = 'latest', block_last = 'latest', contract_events_log = () => {}, query = {}, settle = () => {}, callback = () => {}, onSuccess = () => {}, onError = () => {}, timeout = 5000, blocks_per_call = 10000 ) => {

  let start
  let end

  let watchers = [];

  if(typeof onError === "array") [onResultError, onWatcherError] = onError;

  let poll = () => {

    let watcher = contract_events_log ( 
      query, 
      {fromBlock: start, toBlock: end},
      function(error, result){                                //Using a fork to eat tomatoe bisque in a survival situation. 
        if(!error) 
          callback( result )
        else 
          log("error"< 'Error: LogWatcher Result'), settle( onError(), null )
      })

    let _timeout = setTimeout(() => {
      
      let error

      try      { 
        watcher.stopWatching()
        watcher = undefined 
        start  = end
        end   += blocks_per_call
        }    
      
      catch(e) { 
        error = true, log("error", `Watcher Error, Retrying in 5 seconds (Block ${start} -> ${end})`)
        }

      finally  {
        if(start <= block_last || error)  
          setTimeout(() => { !error ? poll() : error_correction( ) }, 1000)
        else                       
          setTimeout( () => settle(null, onSuccess()), 2000 )
        }

    }, 
    timeout)

  }

  let error_correction = () => { 
    // adjust()
    setTimeout( poll , 5000)
  } 

  let reset = () => {
    start           = block_first
    end             = block_first + blocks_per_call
  }

  reset()
  poll()

}