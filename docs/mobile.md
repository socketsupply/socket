# APIS

Mobile and desktop APIs can be found on a global object named `system`. Almost none of the desktop APIs apply to mobile because the environment is quite different. For example, there is no "main" process on mobile. Mobile APIs are namespaced using the objects `tcp`, `udp` and `utp`.

## TCP

### Server &larr; `tcp.createServer([options])`

Creates a new TCP server.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| options | Object | `{}` | | An optional options object |
| options.simultaneousAccepts | Boolean | true | Enable / disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections. See [libuv][uv0] docs for more info. |

> __Return Value__ `Server` (An instance of an [`EventEmitter`][ee]-like object)

```js
const server = window.system.tcp.createServer()

server.on('connection', socket => {
  socket.on('data', data => {
    console.log(data)
  })

  socket.write("hello")
})

server.listen(9200)
```

<br/>

### Server &sdot; `listen(port[, cb])`

Start a server listening for connections.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| port | Object | | &#10003; | The port on which to allow connections. |
| cb | Function | |  | If supplied, will be added as a listener for the `"connect"` event on the returned socket once. |

> __Return Value__ `none`

<br/>

### Server &sdot; `on(event, cb)`

Adds the listener function to the end of the listeners array for the event named eventName. No checks are made to see if the listener has already been added. Multiple calls passing the same combination of eventName and listener will result in the listener being added, and called, multiple times. See the Node.js [`EventEmitter`][ee] documentation for all methods and properties.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| name | String | | &#10003; | The name of the event (continue reading this document for possible event names that can be observed). |
| cb | Function | | &#10003; | The function to be called when there an event is emitted which matches the event name. |

> __Return Value__ `none`

<br/>

### Server &sdot; `close([cb])`

Stops the server from accepting new connections and keeps existing connections. This function is asynchronous, the server is finally closed when all connections are ended and the server emits a 'close' event. The optional callback will be called once the 'close' event occurs. Unlike that event, it will be called with an Error as its only argument if the server was not open when it was closed.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| cb | Function |  |  | If supplied, will be added as a listener for the `"connect"` event on the returned socket once. |

> __Return Value__ `none`

<br/>

### Server &sdot; `"listening"`

Emitted when the server has been bound after calling the `listen` method.

<br/>

### Server &sdot; `"connection"`

Emitted when a new connection is made. The callback provides a single value, `socket`, which is an instance of `Socket` as described in this document.

<br/>

### Server &sdot; `"data"`

Emitted when data is received. The argument data will be a Buffer or String. Encoding of data is set by the `setEncoding` method. The data will be lost if there is no listener when a Socket emits a 'data' event.

<br/>

### Server &sdot; `"closed"`

Emitted when the connection has fully closed.

<br/>


### Socket &larr; `tcp.createConnection(port[, address][, cb])`

Creates a new socket by immediately initiating a connection.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| port | Object |  | &#10003; | The port to connect with. |
| address | String | |  | An `ipv4` or `ipv6` address. |
| cb | Function | |  | If supplied, will be added as a listener for the `"connect"` event on the returned socket once. |

> __Return Value__ `Socket` (An instance of an [`EventEmitter`][ee]-like object)

```js
const socket = window.system.tcp.createConnection(9200, '192.168.1.22')

socket.on('connect', socket => {
  document.body.style.border = '1px solid green'
});

socket.on('data', data => {
  // data is of type string.
})
```

<br/>

### Socket &sdot; `on(event, cb)`

Adds the listener function to the end of the listeners array for the event named eventName. No checks are made to see if the listener has already been added. Multiple calls passing the same combination of eventName and listener will result in the listener being added, and called, multiple times. See the Node.js [`EventEmitter`][ee] documentation for all methods and properties.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| name | String | | &#10003; | The name of the event (continue reading this document for possible event names that can be observed). |
| cb | Function | | &#10003; | The function to be called when there an event is emitted which matches the event name. |

> __Return Value__ `none`

<br/>

### Socket &sdot; `write([data[, encoding]][, cb])`

Sends data on the socket. The second parameter specifies the encoding in the case of a string. It defaults to UTF8 encoding.

Returns true if the entire data was flushed successfully to the kernel buffer. Returns false if all or part of the data was queued in user memory. 'drain' will be emitted when the buffer is again free.

The optional callback parameter will be executed when the data is finally written out, which may not be immediately.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| data | String | | | The data to send to the socket. |
| encoding | String | `utf8`, `Uint8Array` | | Only used when data is string. |
| cb | Function | |  | If supplied, will be added as a listener for the `"connect"` event on the returned socket once. |

> __Return Value__ `none`

<br/>



### Socket &sdot; `end([data[, encoding]][, cb])`

Half-closes the socket. i.e., it sends a FIN packet. It is possible the server will still send some data.

| Argument | Type | Default | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| data | String | | | The data to send to the socket. |
| encoding | String | `utf8`, `Uint8Array` | | Only used when data is string. |
| cb | Function | |  | If supplied, will be added as a listener for the `"connect"` event on the returned socket once. |

> __Return Value__ `none`

<br/>

### Socket &sdot; `"listening"`

Emitted when the server has been bound after calling the `listen` method.

<br/>

### Socket &sdot; `"data"`

Emitted when data is received. The argument data will be a Buffer or String. Encoding of data is set by the `setEncoding` method. The data will be lost if there is no listener when a Socket emits a 'data' event.

<br/>

### Server &sdot; `"closed"`

Emitted when the connection has fully closed.

<br/>




# GLOBAL EVENTS

The following events are emitted on the `window` object, and can be listened to
with `window.addEventListener`.

### Window &sdot; `"application"`

Emitted when the application is sent to the background or brought to the foreground by the user or other system events.

| Property | Type | Description |
| :--- | :--- | :--- |
| status | String | Possible values are`background` or `foreground`. |

```js
window.addEventListener('application', e => {
  if (e.detail.status === 'background') myFunction()
})
```

<br/>

### Window &sdot; `"network"`

Emitted when there is a change in the status of the network.

| Property | Type | Description |
| :--- | :--- | :--- |
| status | String | Possible values are`online` or `offline`. |
| message | String | A description of why the network status changed. |

```js
window.addEventListener('network', e => {
  if (e.detail.status === 'offline') myFunction()
})
```

<br/>

### Window &sdot; "data"

Emitted any time there is any data from the ipc channel, this
is a kind of firehose of data that can be helpful for debugging.

| Property | Type | Description |
| :--- | :--- | :--- |
| serverId | String? | If the message was send by a server (uint64). |
| clientId | String? | If the message was send by a client (uint64). |
| data | Object? | Could be anything, in the case of a socket message, it will be a base64 encoded string. |

```js
window.addEventListener('data', e => {
  myMessageCount++
})
```

<br/>

[ee]:https://nodejs.org/api/events.html#class-eventemitter
[uv0]:http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_simultaneous_accepts
