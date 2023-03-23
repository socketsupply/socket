
### [`send(options)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L168)

Send event to webview via IPC

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| options.window | number |  | false | window index to send event to |
| options.event | string |  | false | event name |
| options.value | any |  | true | data to send |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Error| undefined> |  |


### [`heartbeat()`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L198)

Send the heartbeat event to the webview.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Error| undefined> |  |


### [`addListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L209)

Adds a listener to the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`on(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L220)

Adds a listener to the window. An alias for `addListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`once(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L230)

Adds a listener to the window. The listener is removed after the first call.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`removeListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L240)

Removes a listener from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |


### [`removeAllListeners(event)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L249)

Removes all listeners from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listeners from |


### [`off(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L260)

Removes a listener from the window. An alias for `removeListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |

