//https://codepen.io/danny_pule/pen/WRgqNx

const convert_to_csv = objArray => {
    let array = typeof objArray != 'object' ? JSON.parse(objArray) : objArray;
    let str = '';

    for (let i = 0; i < array.length; i++) {
        let line = '';
        for (let index in array[i]) {
            if (line != '') line += ','

            line += array[i][index];
        }

        str += line + '\r\n';
    }

    return str;
}


const export_csv = (headers, items, filename) => {

    filename = filename + '.csv' || 'export.csv'

    if (headers) {
        items.unshift(headers)
    }

    let jsonObject = JSON.stringify(items)

    let csv = convert_to_csv(jsonObject)


    let blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' })

    if (navigator.msSaveBlob) { // IE 10+
        navigator.msSaveBlob(blob, filename)
    } else {
        let link = document.createElement("a")
        if (link.download !== undefined) { // feature detection
            // Browsers that support HTML5 download attribute
            let url = URL.createObjectURL(blob)
            link.setAttribute("href", url)
            link.setAttribute("download", filename)
            link.style.visibility = 'hidden'
            document.body.appendChild(link)
            link.click()
            document.body.removeChild(link)
        }
    }

}


const export_log = (logs, filename) => {
    
    filename = filename + '.log' || 'export.log';

    let log = logs.reduce( (acc, log) => { return `${acc}${log}\r\n` } )

    let blob = new Blob([log], { type: 'text/plain;charset=utf-8;' })

    if (navigator.msSaveBlob) { // IE 10+
        navigator.msSaveBlob(blob, filename)
    } else {
        let link = document.createElement("a");
        if (link.download !== undefined) { // feature detection
            // Browsers that support HTML5 download attribute
            let url = URL.createObjectURL(blob)
            link.setAttribute("href", url)
            link.setAttribute("download", filename)
            link.style.visibility = 'hidden'
            document.body.appendChild(link)
            link.click()
            document.body.removeChild(link)
        }
    }

}

const download_snapshot = () => {
  
  let headers = {
      eth:      "eth_address" 
      ,eos:      "eos_key"
      ,balance:  "balance"
  };

  let filename = `eos-snapshot_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`; 

  let snapshot = output.snapshot.map( registrant => { return { eth: registrant.eth, eos: registrant.eos, balance: formatEOS(registrant.balance.total) } } )

  export_csv(headers, snapshot, filename)

}


const download_rejects = () => {
  
  let headers = {
      rejected:     "error"
      ,eth:          "eth_address" 
      ,eos:          "eos_key"
      ,balance:      "balance"
  };

  let filename = `eos-rejections_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  let rejects = output.rejects.map( registrant => { return { error: registrant.error, eth: registrant.eth, eos: registrant.eos, balance: formatEOS(registrant.balance.total)}  } )

  export_csv(headers, rejects, filename)

}

const download_reclaimable = () => {
  
  let headers = {
      eth:          "eth_address" 
      ,tx:          "tx"
      ,balance:      "amount"
  };

  let reclaimable_array = [];
  for( let registrant in output.reclaimable )  {
    output.reclaimable[registrant].forEach( tx => { reclaimable_array.push(tx) })
  }

  let filename = `eos-reclaimable_tx_${ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  let reclaimables = reclaimable_array.map( tx => { return { eth: tx.eth, tx: tx.tx, amount: formatEOS(web3.toBigNumber(tx.amount).div(WAD)) } } )

  export_csv(headers, reclaimables,  filename); 

}

const download_reclaimed = () => {
  
  let headers = {
      eth:          "eth_address" 
      ,eos:         "eos_key"
      ,tx:          "tx"
      ,amount:      "amount"
  };

  let filename = `eos-successfully_reclaimed_tx_${ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  let reclaimed = output.reclaimed.map( tx => { return { eth: tx.eth, eos: tx.eos, tx: tx.tx, amount: formatEOS(web3.toBigNumber(tx.amount).div(WAD)) } } )

  export_csv(headers, reclaimed, filename); 
 

}


const download_logs = () => {

  let filename = `eos-logs_${ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  export_log(output.logs, filename)

}



