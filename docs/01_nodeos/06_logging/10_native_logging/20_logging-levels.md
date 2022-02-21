---
content_title: Logging Levels
---

There are six available logging levels:
- all
- debug
- info
- warn
- error
- off  

Sample `logging.json`:

```
{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ],
        "flush": true
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ],
        "flush": true
      },
      "enabled": true
    },{
      "name": "net",
      "type": "gelf",
      "args": {
        "endpoint": "10.10.10.10:12201",
        "host": "host_name"
      },
      "enabled": true
    },{
      "name": "zip",
      "type": "zipkin",
      "args": {
        "endpoint": "http://127.0.0.1:9411",
        "path": "/api/v2/spans",
        "service_name": "nodeos",
        "timeout_us": 200000
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "zipkin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "zip"
      ]
    },{
      "name": "net_plugin_impl",
      "level": "info",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "http_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "producer_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "transaction_success_tracing",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "transaction_failure_tracing",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "state_history",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "trace_api",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "blockvault_client_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    }
  ]
}
```
