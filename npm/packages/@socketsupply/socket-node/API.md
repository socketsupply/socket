
### [`send(o)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L167)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| o | object |  | false |  |
| o.window | number |  | false |  |
| o.event | string |  | false |  |
| o.value | any |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


### [`heartbeat()`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L196)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


### [`addListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L207)

Adds a listener to the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`on(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L218)

Adds a listener to the window. An alias for `addListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`once(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L228)

Adds a listener to the window. The listener is removed after the first call.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`removeListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L238)

Removes a listener from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |


### [`removeAllListeners(event)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L247)

Removes all listeners from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listeners from |


### [`off(event, cb)`](https://github.com/socketsupply/socket/blob/master/npm/packages/@socketsupply/socket-node/index.js#L258)

Removes a listener from the window. An alias for `removeListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |

