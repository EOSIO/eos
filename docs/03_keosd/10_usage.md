
[[info | Recommended Usage]]
| For most users, the easiest way to use `kapifinyd` is to have `clapifiny` launch it automatically. Wallet files will be created in the default directory (`~/apifiny-wallet`).

## Launching kapifinyd manually

`kapifinyd` can be launched manually from the terminal by running:

```sh
$ kapifinyd
```

By default, `kapifinyd` creates the folder `~/apifiny-wallet` and populates it with a basic `config.ini` file.  The location of the config file can be specified on the command line using the `--config-dir` argument.  The configuration file contains the HTTP server endpoint for incoming HTTP connections and other parameters for cross-origin resource sharing.

[[info | Wallet Location]]
| The location of the wallet data folder can be specified on the command line with the `--data-dir` option.

## Auto-locking

By default, `kapifinyd` is set to lock your wallet after 15 minutes of inactivity. This is configurable in the `config.ini` by setting the timeout seconds in `unlock-timeout`. Setting it to 0 will cause `kapifinyd` to always lock your wallet.

## Stopping kapifinyd

The most effective way to stop `kapifinyd` is to find the kapifinyd process and send a SIGTERM signal to it.

## Other options

For a list of all commands known to `kapifinyd`, simply run it with no arguments:

```sh
$ kapifinyd --help
```

```console
Application Options:

Config Options for apifiny::http_plugin:
  --unix-socket-path arg (=kapifinyd.sock)  The filename (relative to data-dir) to
                                        create a unix socket for HTTP RPC; set
                                        blank to disable.
  --http-server-address arg             The local IP and port to listen for
                                        incoming http connections; leave blank
                                        to disable.
  --https-server-address arg            The local IP and port to listen for
                                        incoming https connections; leave blank
                                        to disable.
  --https-certificate-chain-file arg    Filename with the certificate chain to
                                        present on https connections. PEM
                                        format. Required for https.
  --https-private-key-file arg          Filename with https private key in PEM
                                        format. Required for https
  --https-ecdh-curve arg (=secp384r1)   Configure https ECDH curve to use:
                                        secp384r1 or prime256v1
  --access-control-allow-origin arg     Specify the Access-Control-Allow-Origin
                                        to be returned on each request.
  --access-control-allow-headers arg    Specify the Access-Control-Allow-Header
                                        s to be returned on each request.
  --access-control-max-age arg          Specify the Access-Control-Max-Age to
                                        be returned on each request.
  --access-control-allow-credentials    Specify if Access-Control-Allow-Credent
                                        ials: true should be returned on each
                                        request.
  --max-body-size arg (=1048576)        The maximum body size in bytes allowed
                                        for incoming RPC requests
  --http-max-bytes-in-flight-mb arg (=500)
                                        Maximum size in megabytes http_plugin
                                        should use for processing http
                                        requests. 503 error response when
                                        exceeded.
  --verbose-http-errors                 Append the error log to HTTP responses
  --http-validate-host arg (=1)         If set to false, then any incoming
                                        "Host" header is considered valid
  --http-alias arg                      Additionaly acceptable values for the
                                        "Host" header of incoming HTTP
                                        requests, can be specified multiple
                                        times.  Includes http/s_server_address
                                        by default.
  --http-threads arg (=2)               Number of worker threads in http thread
                                        pool

Config Options for apifiny::wallet_plugin:
  --wallet-dir arg (=".")               The path of the wallet files (absolute
                                        path or relative to application data
                                        dir)
  --unlock-timeout arg (=900)           Timeout for unlocked wallet in seconds
                                        (default 900 (15 minutes)). Wallets
                                        will automatically lock after specified
                                        number of seconds of inactivity.
                                        Activity is defined as any wallet
                                        command e.g. list-wallets.
  --yubihsm-url URL                     Override default URL of
                                        http://localhost:12345 for connecting
                                        to yubihsm-connector
  --yubihsm-authkey key_num             Enables YubiHSM support using given
                                        Authkey

Application Config Options:
  --plugin arg                          Plugin(s) to enable, may be specified
                                        multiple times

Application Command Line Options:
  -h [ --help ]                         Print this help message and exit.
  -v [ --version ]                      Print version information.
  --print-default-config                Print default configuration template
  -d [ --data-dir ] arg                 Directory containing program runtime
                                        data
  --config-dir arg                      Directory containing configuration
                                        files such as config.ini
  -c [ --config ] arg (=config.ini)     Configuration file name relative to
                                        config-dir
  -l [ --logconf ] arg (=logging.json)  Logging configuration file name/path
                                        for library users
```
