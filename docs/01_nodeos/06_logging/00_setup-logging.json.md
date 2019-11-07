The `logging.json` file is usually located in the specified `--config-dir`, the same directory as the `config.ini` file. This path can be explicitly defined using the `-l` or `--logconf` options when starting `nodeos`.
 
```sh
./nodeos --help
  ...
  Application Command Line Options:
  ...
  --config-dir arg                      Directory containing configuration files such as config.ini
  -l [ --logconf ] arg (=logging.json)  Logging configuration file name/path for library users

```
