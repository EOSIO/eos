// CSVs
const download_reclaimable_csv = () => { 
  let headers = {
    eth:          "eth_address" 
    ,tx:          "tx"
    ,balance:      "amount"
  };
  export_csv(headers, output.reclaimable,  generate_filename('reclaimable-tx'))
}

const download_reclaimed_csv = () => {
  let headers = {
    eth:          "eth_address" 
    ,eos:         "eos_key"
    ,tx:          "tx"
    ,amount:      "amount"
  };
  export_csv(headers, output.reclaimed, generate_filename('successfully-reclaimed-tx') ); 
}

const download_snapshot_csv = () => {
  let headers = {
    eth:       "eth_address" 
    ,eos:      "eos_key"
    ,balance:  "balance"
  };
  export_csv(headers, output.snapshot, generate_filename('snapshot') )
}

const download_rejects_csv = () => {
  let headers = {
    rejected:      "error"
    ,eth:          "eth_address" 
    ,eos:          "eos_key"
    ,balance:      "balance"
  };
  export_csv(headers, output.rejects, generate_filename('rejections') )
}

// JSON
const download_reclaimable_json = ()  => export_json(output.reclaimable, generate_filename('reclaimable-tx'))
const download_reclaimed_json   = ()  => export_json(output.reclaimed,   generate_filename('successfully-reclaimed-tx') )
const download_snapshot_json    = ()  => export_json(output.snapshot,    generate_filename('snapshot') )
const download_rejects_json     = ()  => export_json(output.rejects,     generate_filename('rejections') )

// Logs
const download_logs = ()              => export_log(output.logs,  generate_filename('logs'))

// Helper
const generate_filename = ( type )    => `eos-${type}_${SNAPSHOT_ENV}_${SS_LAST_BLOCK}-${SS_FIRST_BLOCK}`

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

const export_json = (items, filename) => {
  filename = filename + '.json' || 'export.csv'
  let json = JSON.stringify(items)
  let blob = new Blob([json], { type: 'application/json;charset=utf-8;' })
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
