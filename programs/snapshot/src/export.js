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

const download_reclaimable = () => {
  
  let headers = {
      eth:          "eth_address" 
      ,tx:          "tx"
      ,balance:      "amount"
  };

  let filename = `eos-reclaimable_tx_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  export_csv(headers, output.reclaimable,  filename); 

}

const download_reclaimed = () => {

  let headers = {
      eth:          "eth_address" 
      ,eos:         "eos_key"
      ,tx:          "tx"
      ,amount:      "amount"
  };

  let filename = `eos-successfully_reclaimed_tx_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  let reclaimed = output.reclaimed

  
  export_csv(headers, reclaimed, filename); 

}

const download_snapshot = () => {
  
  let headers = {
      eth:      "eth_address" 
      ,eos:      "eos_key"
      ,balance:  "balance"
  };

  let filename = `eos-snapshot_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`; 

  export_csv(headers, output.snapshot, filename)

}

const download_rejects = () => {
  
  let headers = {
      rejected:     "error"
      ,eth:          "eth_address" 
      ,eos:          "eos_key"
      ,balance:      "balance"
  };

  let filename = `eos-rejections_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  export_csv(headers, output.rejects, filename)

}

const download_logs = () => {

  let filename = `eos-logs_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

  export_log(output.logs, filename)

}