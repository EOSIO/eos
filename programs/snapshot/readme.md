# About
web3js based script that generates a token distribution list.

1. Compiles the ethereum address registry based on the LogRegister contract event.
1. Compiles a list of orphaned tokens accidentally sent to the **token** and **crowdsale** contract. 
1. For each registrant, it then...
  1. Retrieves EOS public key from `keys` constant function in EOS Crowdsale contract.
  1. Attempts to fix key if there appears to be an correctable mistake.
  1. Calculates the total balance...
    1. Obtains the ethereum wallet balance
    1. Obtains the unclaimed token balanace
    1. Checks to see if the current ethereum address has any orphaned funs
    1. Adds them all together
1. Validates the entry and if it validates, it's addede to the distribution.
1. If the registrant doesn't validate due to a zero balance or invalid EOS key, they are saved to later reference but discluded from the distribution. 

# Prerequisites
- A Local Ethereuym Node, **parity** suggested
- A modern browser
- **Turn off** MetaMask if you have it running
- Python or means to run simple webserver

# Usage

## On Your Machine
1. Checkout or download this repository
1. Navigate to the project directory
1. Start a web server. For example, you could run `python -m SimpleHTTPServer 8000` in the project directory to start a webserver at **http://localhost:8000**.
1. Start your parity or geth node with flag `--rpccorsdomain 'http://localhost:8000'` ... If you didn't use port `8000`, change accordingly. 
1. Navigate to [http://localhost:8000](http://localhost:8000) (if you used a different port, adjust accordingly)
1. Open developer console for some information, `debug.output()` will give you some calculations
1. When complete, links to download the snapshot and other datasets will be made available.

# Config

Some options in `src/config.js`

## User Config

1. `NODE` Change according to configuration (default: http://localhost:8345)
1. `CONSOLE_LOGGING` Will output logs snapshot into developer console, turn off if you want better performance (default:true)
2. `VERBOSE_REGISTRY_LOGS` Outputs every single registrant into developer logs, very slow but works in Chrome. Will break firefox. (default:false)

## Snapshot

The snapshot requires a range for output consistency

1. `SS_FIRST_BLOCK` Presently set to block *3904416* or the block with first transaction to the crowdsale contract. 
2. `SS_LAST_BLOCK` TBA
3. `SNAPSHOT_ENV` Determines filename output.



# Notes
1. Script can run in unfocused tab.
2. ES6, no babel/shims are included in this package. 
1. Don't do anything crazy in your browser while running this script.
1. If the script hits an error it will attempt to restart itself. 
