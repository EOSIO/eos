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
     "name": "consoleout", 
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
       ]
     },
     "enabled": true
   },{
     "name": "net",
     "type": "gelf",
     "args": {
       "endpoint": "10.10.10.10",
       "host": "test"
     },
     "enabled": true
   }
 ],
 "loggers": [{
     "name": "default",
     "level": "info",
     "enabled": true,
     "additivity": false,
     "appenders": [
       "consoleout",
       "net"
     ]
   },{
     "name": "net_plugin_impl",
     "level": "debug",
     "enabled": true,
     "additivity": false,
     "appenders": [
       "net"
     ]
   }
 ]
}
```
