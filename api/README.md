# [Application](https://github.com/socketsupply/socket/blob/master/api/application.js#L13)


 Provides Application level methods

 Example usage:
 ```js
 import { createWindow } from 'socket:application'
 ```

## [`createWindow(opts)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L37)

Creates a new window and returns an instance of ApplicationWindow.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.index | number |  | false | the index of the window |
| opts.path | string |  | false | the path to the HTML file to load into the window |
| opts.title | string |  | true | the title of the window |
| opts.width | number \| string |  | true | the width of the window. If undefined, the window will have the main window width. |
| opts.height | number \| string |  | true | the height of the window. If undefined, the window will have the main window height. |
| opts.minWidth | number \| string | 0 | true | the minimum width of the window |
| opts.minHeight | number \| string | 0 | true | the minimum height of the window |
| opts.maxWidth | number \| string | 100% | true | the maximum width of the window |
| opts.maxHeight | number \| string | 100% | true | the maximum height of the window |
| opts.resizable | boolean | true | true | whether the window is resizable |
| opts.frameless | boolean | false | true | whether the window is frameless |
| opts.utility | boolean | false | true | whether the window is utility (macOS only) |
| opts.canExit | boolean | false | true | whether the window can exit the app |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ApplicationWindow> |  |

## [`getScreenSize()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L95)

Returns the current screen size.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

## [`getWindows(indices)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L115)

Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| indices | number |  | true | the indices of the windows |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Object.<number, ApplicationWindow>> |  |

## [`getWindow(index)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L134)

Returns the ApplicationWindow instance for the given index

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| index | number |  | false | the index of the window |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ApplicationWindow> | the ApplicationWindow instance or null if the window does not exist |

## [`getCurrentWindow()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L144)

Returns the ApplicationWindow instance for the current window.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ApplicationWindow> |  |

## [`exit(code)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L153)

Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| code | number | 0 | true | an exit code |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

## [`setSystemMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L250)

Set the native menu for the app.


 Socket Runtime provides a minimalist DSL that makes it easy to create
 cross platform native system and context menus.

 Menus are created at run time. They can be created from either the Main or
 Render process. The can be recreated instantly by calling the `setSystemMenu` method.

 The method takes a string. Here's an example of a menu. The semi colon is
 significant indicates the end of the menu. Use an underscore when there is no
 accelerator key. Modifiers are optional. And well known OS menu options like
 the edit menu will automatically get accelerators you dont need to specify them.


 ```js
 socket.application.setSystemMenu({ index: 0, value: `
   App:
     Foo: f;

   Edit:
     Cut: x
     Copy: c
     Paste: v
     Delete: _
     Select All: a;

   Other:
     Apple: _
     Another Test: T
     !Im Disabled: I
     Some Thing: S + Meta
     ---
     Bazz: s + Meta, Control, Alt;
 `)
 ```

 Separators

 To create a separator, use three dashes `---`.


 Accelerator Modifiers

 Accelerator modifiers are used as visual indicators but don't have a
 material impact as the actual key binding is done in the event listener.

 A capital letter implies that the accelerator is modified by the `Shift` key.

 Additional accelerators are `Meta`, `Control`, `Option`, each separated
 by commas. If one is not applicable for a platform, it will just be ignored.

 On MacOS `Meta` is the same as `Command`.


 Disabled Items

 If you want to disable a menu item just prefix the item with the `!` character.
 This will cause the item to appear disabled when the system menu renders.


 Submenus

 We feel like nested menus are an anti-pattern. We don't use them. If you have a
 strong argument for them and a very simple pull request that makes them work we
 may consider them.


 Event Handling

 When a menu item is activated, it raises the `menuItemSelected` event in
 the front end code, you can then communicate with your backend code if you
 want from there.

 For example, if the `Apple` item is selected from the `Other` menu...

 ```js
 window.addEventListener('menuItemSelected', event => {
   assert(event.detail.parent === 'Other')
   assert(event.detail.title === 'Apple')
 })
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |
| options.value | string |  | false | the menu layout |
| options.index | number |  | false | the window to target (if applicable) |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

## [`setSystemMenuItemEnabled(value)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L331)

Set the enabled state of the system menu.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| value | object |  | false | an options object |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

## [runtimeVersion](https://github.com/socketsupply/socket/blob/master/api/application.js#L339)

Socket Runtime version.

## [debug](https://github.com/socketsupply/socket/blob/master/api/application.js#L345)

Runtime debug flag.

## [config](https://github.com/socketsupply/socket/blob/master/api/application.js#L351)

Application configuration.

## [backend](https://github.com/socketsupply/socket/blob/master/api/application.js#L356)

The application's backend instance.

### [`open(opts)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L362)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.force | boolean | false | true | whether to force the existing process to close |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`close()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L370)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


# [Bluetooth](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L12)


 A high-level, cross-platform API for Bluetooth Pub-Sub

 Example usage:
 ```js
 import { Bluetooth } from 'socket:bluetooth'
 ```

## [`Bluetooth` (extends `EventEmitter`)](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L32)

Create an instance of a Bluetooth service.

### [`constructor(serviceId)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L40)

constructor is an example property that is set to `true`
 Creates a new service with key-value pairs

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| serviceId | string |  | false | Given a default value to determine the type |

### [`start()`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L90)

Start the Bluetooth service.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`subscribe(id)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L119)

Start scanning for published values that correspond to a well-known UUID.
 Once subscribed to a UUID, events that correspond to that UUID will be
 emitted. To receive these events you can add an event listener, for example...

 ```js
 const ble = new Bluetooth(id)
 ble.subscribe(uuid)
 ble.on(uuid, (data, details) => {
   // ...do something interesting
 })
 ```


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| id | string |  | true | A well-known UUID |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`publish(id, value)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L142)

Start advertising a new value for a well-known UUID

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| id | string |  | true | A well-known UUID |
| value | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |



# [Buffer](https://github.com/socketsupply/socket/blob/master/api/buffer.js)

Buffer module is a [third party](https://github.com/feross/buffer) vendor module provided by Feross Aboukhadijeh and other contributors (MIT License).

External docs: https://nodejs.org/api/buffer.html

# [Crypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L14)


 Some high-level methods around the `crypto.subtle` API for getting
 random bytes and hashing.

 Example usage:
 ```js
 import { randomBytes } from 'socket:crypto'
 ```

## [webcrypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L27)

External docs: https://developer.mozilla.org/en-US/docs/Web/API/Crypto
WebCrypto API

## [ready](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L56)

A promise that resolves when all internals to be loaded/ready.

## [`getRandomValues(buffer)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L71)

External docs: https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues
Generate cryptographically strong random values into the `buffer`

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| buffer | TypedArray |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | TypedArray |  |

## [`rand64()`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L94)

Generate a random 64-bit number.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| A random 64-bit number. | BigInt |  |

## [RANDOM_BYTES_QUOTA](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L102)

Maximum total size of random bytes per page

## [MAX_RANDOM_BYTES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L107)

Maximum total size for random bytes.

## [MAX_RANDOM_BYTES_PAGES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L112)

Maximum total amount of allocated per page of bytes (max/quota)

## [`randomBytes(size)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L120)

Generate `size` random bytes.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The number of bytes to generate. The size must not be larger than 2**31 - 1. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Buffer | A promise that resolves with an instance of socket.Buffer with random bytes. |

## [`createDigest(algorithm, message)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L147)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| algorithm | string |  | false | `SHA-1` | `SHA-256` | `SHA-384` | `SHA-512` |
| message | Buffer \| TypedArray \| DataView |  | false | An instance of socket.Buffer, TypedArray or Dataview. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Buffer> | A promise that resolves with an instance of socket.Buffer with the hash. |


# [DNS](https://github.com/socketsupply/socket/blob/master/api/dns/index.js#L17)


 This module enables name resolution. For example, use it to look up IP
 addresses of host names. Although named for the Domain Name System (DNS),
 it does not always use the DNS protocol for lookups. dns.lookup() uses the
 operating system facilities to perform name resolution. It may not need to
 perform any network communication. To perform name resolution the way other
 applications on the same system do, use dns.lookup().

 Example usage:
 ```js
 import { lookup } from 'socket:dns'
 ```

## [`lookup(hostname, options, cb)`](https://github.com/socketsupply/socket/blob/master/api/dns/index.js#L60)

External docs: https://nodejs.org/api/dns.html#dns_dns_lookup_hostname_options_callback
Resolves a host name (e.g. `example.org`) into the first found A (IPv4) or
 AAAA (IPv6) record. All option properties are optional. If options is an
 integer, then it must be 4 or 6 – if options is 0 or not provided, then IPv4
 and IPv6 addresses are both returned if found.

 From the node.js website...

 > With the all option set to true, the arguments for callback change to (err,
 addresses), with addresses being an array of objects with the properties
 address and family.

 > On error, err is an Error object, where err.code is the error code. Keep in
 mind that err.code will be set to 'ENOTFOUND' not only when the host name does
 not exist but also when the lookup fails in other ways such as no available
 file descriptors. dns.lookup() does not necessarily have anything to do with
 the DNS protocol. The implementation uses an operating system facility that
 can associate names with addresses and vice versa. This implementation can
 have subtle but important consequences on the behavior of any Node.js program.
 Please take some time to consult the Implementation considerations section
 before using dns.lookup().


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| hostname | string |  | false | The host name to resolve. |
| options | object \| intenumberger |  | true | An options object or record family. |
| options.family | number \| string | 0 | true | The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0. |
| cb | function |  | false | The function to call after the method is complete. |


# [DNS.promises](https://github.com/socketsupply/socket/blob/master/api/dns/promises.js#L17)


 This module enables name resolution. For example, use it to look up IP
 addresses of host names. Although named for the Domain Name System (DNS),
 it does not always use the DNS protocol for lookups. dns.lookup() uses the
 operating system facilities to perform name resolution. It may not need to
 perform any network communication. To perform name resolution the way other
 applications on the same system do, use dns.lookup().

 Example usage:
 ```js
 import { lookup } from 'socket:dns/promises'
 ```

## [`lookup(hostname, opts)`](https://github.com/socketsupply/socket/blob/master/api/dns/promises.js#L37)

External docs: https://nodejs.org/api/dns.html#dnspromiseslookuphostname-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| hostname | string |  | false | The host name to resolve. |
| opts | Object |  | true | An options object. |
| opts.family | number \| string | 0 | true | The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise |  |


# [Dgram](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L13)


 This module provides an implementation of UDP datagram sockets. It does
 not (yet) provide any of the multicast methods or properties.

 Example usage:
 ```js
 import { createSocket } from 'socket:dgram'
 ```

## [`createSocket(options, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L623)

Creates a `Socket` instance.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | string \| Object |  | false | either a string ('udp4' or 'udp6') or an options object |
| options.type | string |  | true | The family of socket. Must be either 'udp4' or 'udp6'. Required. |
| options.reuseAddr | boolean | false | true | When true socket.bind() will reuse the address, even if another process has already bound a socket on it. Default: false. |
| options.ipv6Only | boolean | false | true | Default: false. |
| options.recvBufferSize | number |  | true | Sets the SO_RCVBUF socket value. |
| options.sendBufferSize | number |  | true | Sets the SO_SNDBUF socket value. |
| options.signal | AbortSignal |  | true | An AbortSignal that may be used to close a socket. |
| callback | function |  | true | Attached as a listener for 'message' events. Optional. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Socket |  |

## [`Socket` (extends `EventEmitter`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L629)

New instances of dgram.Socket are created using dgram.createSocket().
 The new keyword is not to be used to create dgram.Socket instances.

### [`bind(port, address, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L704)

External docs: https://nodejs.org/api/dgram.html#socketbindport-address-callback
Listen for datagram messages on a named port and optional address
 If the address is not specified, the operating system will attempt to
 listen on all addresses. Once the binding is complete, a 'listening'
 event is emitted and the optional callback function is called.

 If binding fails, an 'error' event is emitted.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| port | number |  | false | The port to listen for messages on |
| address | string |  | false | The address to bind to (0.0.0.0) |
| callback | function |  | false | With no parameters. Called when binding is complete. |

### [`connect(port, host, connectListener)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L757)

External docs: https://nodejs.org/api/dgram.html#socketconnectport-address-callback
Associates the dgram.Socket to a remote address and port. Every message sent
 by this handle is automatically sent to that destination. Also, the socket
 will only receive messages from that remote peer. Trying to call connect()
 on an already connected socket will result in an ERR_SOCKET_DGRAM_IS_CONNECTED
 exception. If the address is not provided, '0.0.0.0' (for udp4 sockets) or '::1'
 (for udp6 sockets) will be used by default. Once the connection is complete,
 a 'connect' event is emitted and the optional callback function is called.
 In case of failure, the callback is called or, failing this, an 'error' event
 is emitted.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| port | number |  | false | Port the client should connect to. |
| host | string |  | true | Host the client should connect to. |
| connectListener | function |  | true | Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once. |

### [`disconnect()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L792)

External docs: https://nodejs.org/api/dgram.html#socketdisconnect
A synchronous function that disassociates a connected dgram.Socket from
 its remote address. Trying to call disconnect() on an unbound or already
 disconnected socket will result in an ERR_SOCKET_DGRAM_NOT_CONNECTED exception.


### [`send(msg, offset, length, port, address, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L851)

External docs: https://nodejs.org/api/dgram.html#socketsendmsg-offset-length-port-address-callback
Broadcasts a datagram on the socket. For connectionless sockets, the
 destination port and address must be specified. Connected sockets, on the
 other hand, will use their associated remote endpoint, so the port and
 address arguments must not be set.

 > The msg argument contains the message to be sent. Depending on its type,
 different behavior can apply. If msg is a Buffer, any TypedArray, or a
 DataView, the offset and length specify the offset within the Buffer where
 the message begins and the number of bytes in the message, respectively.
 If msg is a String, then it is automatically converted to a Buffer with
 'utf8' encoding. With messages that contain multi-byte characters, offset,
 and length will be calculated with respect to byte length and not the
 character position. If msg is an array, offset and length must not be
 specified.

 > The address argument is a string. If the value of the address is a hostname,
 DNS will be used to resolve the address of the host. If the address is not
 provided or otherwise nullish, '0.0.0.0' (for udp4 sockets) or '::1'
 (for udp6 sockets) will be used by default.

 > If the socket has not been previously bound with a call to bind, the socket
 is assigned a random port number and is bound to the "all interfaces"
 address ('0.0.0.0' for udp4 sockets, '::1' for udp6 sockets.)

 > An optional callback function may be specified as a way of reporting DNS
 errors or for determining when it is safe to reuse the buf object. DNS
 lookups delay the time to send for at least one tick of the Node.js event
 loop.

 > The only way to know for sure that the datagram has been sent is by using a
 callback. If an error occurs and a callback is given, the error will be
 passed as the first argument to the callback. If a callback is not given,
 the error is emitted as an 'error' event on the socket object.

 > Offset and length are optional but both must be set if either is used.
 They are supported only when the first argument is a Buffer, a TypedArray,
 or a DataView.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| msg | Buffer \| TypedArray \| DataView \| string \| Array |  | false | Message to be sent. |
| offset | integer |  | true | Offset in the buffer where the message starts. |
| length | integer |  | true | Number of bytes in the message. |
| port | integer |  | true | Destination port. |
| address | string |  | true | Destination host name or IP address. |
| callback | Function |  | true | Called when the message has been sent. |

### [`close(callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L931)

External docs: https://nodejs.org/api/dgram.html#socketclosecallback
Close the underlying socket and stop listening for data on it. If a
 callback is provided, it is added as a listener for the 'close' event.



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| callback | function |  | true | Called when the connection is completed or on error. |

### [`address()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L997)

External docs: https://nodejs.org/api/dgram.html#socketaddress
Returns an object containing the address information for a socket. For
 UDP sockets, this object will contain address, family, and port properties.

 This method throws EBADF if called on an unbound socket.


| Return Value | Type | Description |
| :---         | :--- | :---        |
| socketInfo | Object | Information about the local socket |
| socketInfo.address | string | The IP address of the socket |
| socketInfo.port | string | The port of the socket |
| socketInfo.family | string | The IP family of the socket |

### [`remoteAddress()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1032)

External docs: https://nodejs.org/api/dgram.html#socketremoteaddress
Returns an object containing the address, family, and port of the remote
 endpoint. This method throws an ERR_SOCKET_DGRAM_NOT_CONNECTED exception
 if the socket is not connected.


| Return Value | Type | Description |
| :---         | :--- | :---        |
| socketInfo | Object | Information about the remote socket |
| socketInfo.address | string | The IP address of the socket |
| socketInfo.port | string | The port of the socket |
| socketInfo.family | string | The IP family of the socket |

### [`setRecvBufferSize(size)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1063)

External docs: https://nodejs.org/api/dgram.html#socketsetrecvbuffersizesize
Sets the SO_RCVBUF socket option. Sets the maximum socket receive buffer in
 bytes.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The size of the new receive buffer |

### [`setSendBufferSize(size)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1080)

External docs: https://nodejs.org/api/dgram.html#socketsetsendbuffersizesize
Sets the SO_SNDBUF socket option. Sets the maximum socket send buffer in
 bytes.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The size of the new send buffer |

### [`getRecvBufferSize()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1093)

External docs: https://nodejs.org/api/dgram.html#socketgetrecvbuffersize


### [`getSendBufferSize()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1101)

External docs: https://nodejs.org/api/dgram.html#socketgetsendbuffersize


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | the SO_SNDBUF socket send buffer size in bytes. |

### [`code()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1169)



## [`ERR_SOCKET_ALREADY_BOUND` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1175)

Thrown when a socket is already bound.

## [`ERR_SOCKET_DGRAM_IS_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1192)

Thrown when the socket is already connected.

## [`ERR_SOCKET_DGRAM_NOT_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1199)

Thrown when the socket is not connected.

## [`ERR_SOCKET_DGRAM_NOT_RUNNING` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1207)

Thrown when the socket is not running (not bound or connected).

## [`ERR_SOCKET_BAD_TYPE` (extends `TypeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1214)

Thrown when a bad socket type is used in an argument.

## [`ERR_SOCKET_BAD_PORT` (extends `RangeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1224)

Thrown when a bad port is given.



# [Events](https://github.com/socketsupply/socket/blob/master/api/events.js)

Events module is a [third party](https://github.com/browserify/events/blob/main/events.js) module provided by Browserify and Node.js contributors (MIT License).

External docs: https://nodejs.org/api/events.html

# [FS](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L26)


 This module enables interacting with the file system in a way modeled on
 standard POSIX functions.

 The Application Sandbox restricts access to the file system.

 iOS Application Sandboxing has a set of rules that limits access to the file
 system. Apps can only access files in their own sandboxed home directory.

 | Directory | Description |
 | --- | --- |
 | `Documents` | The app’s sandboxed documents directory. The contents of this directory are backed up by iTunes and may be set as accessible to the user via iTunes when `UIFileSharingEnabled` is set to `true` in the application's `info.plist`. |
 | `Library` | The app’s sandboxed library directory. The contents of this directory are synchronized via iTunes (except the `Library/Caches` subdirectory, see below), but never exposed to the user. |
 | `Library/Caches` | The app’s sandboxed caches directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. It's a good place to store data which provides a good offline-first experience for the user. |
 | `Library/Preferences` | The app’s sandboxed preferences directory. The contents of this directory are synchronized via iTunes. Its purpose is to be used by the Settings app. Avoid creating your own files in this directory. |
 | `tmp` | The app’s sandboxed temporary directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. Although, it's recommended that you delete data that is not necessary anymore manually to minimize the space your app takes up on the file system. Use this directory to store data that is only useful during the app runtime. |

 Example usage:
 ```js
 import * as fs from 'socket:fs';
 ```

## [`access(path, mode, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L84)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback
Asynchronously check access a file for a given mode calling `callback`
 upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string? \| function(Error?)? | F_OK(0) | true |  |
| callback | function(Error?)? |  | true |  |

## [`chmod(path, mode, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L117)

External docs: https://nodejs.org/api/fs.html#fschmodpath-mode-callback
Asynchronously changes the permissions of a file.
 No arguments other than a possible exception are given to the completion callback



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |
| callback | function(Error?) |  | false |  |

## [`close(fd, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L166)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsclosefd-callback
Asynchronously close a file descriptor calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| callback | function(Error?)? |  | true |  |

## [`copyFile(src, dest, flags, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L190)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscopyfilesrc-dest-mode-callback
Asynchronously copies `src` to `dest` calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| src | string |  | false | The source file path. |
| dest | string |  | false | The destination file path. |
| flags | number |  | false | Modifiers for copy operation. |
| callback | function(Error=) |  | true | The function to call after completion. |

## [`createReadStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L218)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | ReadStream |  |

## [`createWriteStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L258)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | WriteStream |  |

## [`fstat(fd, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L302)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsfstatfd-options-callback
Invokes the callback with the <fs.Stats> for the file descriptor. See
 the POSIX fstat(2) documentation for more detail.



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false | A file descriptor. |
| options | object? \| function? |  | true | An options object. |
| callback | function? |  | true | The function to call after completion. |

## [`open(path, flags, mode, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L407)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback
Asynchronously open a file calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| flags | string? | r | true |  |
| mode | string? | 0o666 | true |  |
| options | object? \| function? |  | true |  |
| callback | function(Error?, number?)? |  | true |  |

## [`opendir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L460)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback
Asynchronously open a directory calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? \| function(Error?, Dir?) |  | true |  |
| options.encoding | string? | utf8 | true |  |
| options.withFileTypes | boolean? | false | true |  |
| callback | function(Error?, Dir?)? |  | false |  |

## [`read(fd, buffer, offset, length, position, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L486)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreadfd-buffer-offset-length-position-callback
Asynchronously read from an open file descriptor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| buffer | object \| Buffer \| TypedArray |  | false | The buffer that the data will be written to. |
| offset | number |  | false | The position in buffer to write the data to. |
| length | number |  | false | The number of bytes to read. |
| position | number \| BigInt \| null |  | false | Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged. |
| callback | function(Error?, number?, Buffer?) |  | false |  |

## [`readdir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L520)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback
Asynchronously read all entries in a directory.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL  |  | false |  |
| options | object? \| function(Error?, object) |  | true |  |
| options.encoding ? utf8 | string? |  | true |  |
| options.withFileTypes ? false | boolean? |  | true |  |
| callback | function(Error?, object) |  | false |  |

## [`readFile(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L571)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false |  |
| options | object? \| function(Error?, Buffer?) |  | true |  |
| options.encoding ? utf8 | string? |  | true |  |
| options.flag ? r | string? |  | true |  |
| options.signal | AbortSignal? |  | true |  |
| callback | function(Error?, Buffer?) |  | false |  |

## [`stat(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L690)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false | filename or file descriptor |
| options | object? |  | false |  |
| options.encoding ? utf8 | string? |  | true |  |
| options.flag ? r | string? |  | true |  |
| options.signal | AbortSignal? |  | true |  |
| callback | function(Error?, Stats?) |  | false |  |

## [`writeFile(path, data, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L786)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false | filename or file descriptor |
| data | string \| Buffer \| TypedArray \| DataView \| object  |  | false |  |
| options | object? |  | false |  |
| options.encoding ? utf8 | string? |  | true |  |
| options.mode ? 0o666 | string? |  | true |  |
| options.flag ? w | string? |  | true |  |
| options.signal | AbortSignal? |  | true |  |
| callback | function(Error?) |  | false |  |


# [FS.promises](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L25)


  * This module enables interacting with the file system in a way modeled on
 standard POSIX functions.

 The Application Sandbox restricts access to the file system.

 iOS Application Sandboxing has a set of rules that limits access to the file
 system. Apps can only access files in their own sandboxed home directory.

 | Directory | Description |
 | --- | --- |
 | `Documents` | The app’s sandboxed documents directory. The contents of this directory are backed up by iTunes and may be set as accessible to the user via iTunes when `UIFileSharingEnabled` is set to `true` in the application's `info.plist`. |
 | `Library` | The app’s sandboxed library directory. The contents of this directory are synchronized via iTunes (except the `Library/Caches` subdirectory, see below), but never exposed to the user. |
 | `Library/Caches` | The app’s sandboxed caches directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. It's a good place to store data which provides a good offline-first experience for the user. |
 | `Library/Preferences` | The app’s sandboxed preferences directory. The contents of this directory are synchronized via iTunes. Its purpose is to be used by the Settings app. Avoid creating your own files in this directory. |
 | `tmp` | The app’s sandboxed temporary directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. Although, it's recommended that you delete data that is not necessary anymore manually to minimize the space your app takes up on the file system. Use this directory to store data that is only useful during the app runtime. |

 Example usage:
 ```js
 import fs from 'socket:fs/promises'
 ```

## [`access(path, mode, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L86)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesaccesspath-mode
Asynchronously check access a file.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string? |  | true |  |
| options | object? |  | true |  |

## [`chmod(path, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L96)

External docs: https://nodejs.org/api/fs.html#fspromiseschmodpath-mode


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

## [`mkdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L208)

Asynchronously creates a directory.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | String |  | false | The path to create |
| options | Object |  | false | The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<any> | Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true. |

## [`open(path, flags, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L236)

External docs: https://nodejs.org/api/fs.html#fspromisesopenpath-flags-mode
Asynchronously open a file.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| flags | string |  | false | default: 'r' |
| mode | string |  | false | default: 0o666 |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<FileHandle> |  |

## [`opendir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L248)

External docs: https://nodejs.org/api/fs.html#fspromisesopendirpath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? |  | true |  |
| options.encoding | string? | utf8 | true |  |
| options.bufferSize | number? | 32 | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Dir> |  |

## [`readdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L260)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreaddirpath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? |  | false |  |
| options.encoding | string? | utf8 | true |  |
| options.withFileTypes | boolean? | false | true |  |

## [`readFile(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L293)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreadfilepath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string |  | false |  |
| options | object? |  | true |  |
| options.encoding | (string \| null)? | null | true |  |
| options.flag | string? | r | true |  |
| options.signal | AbortSignal? |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Buffer \| string> |  |

## [`stat(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L380)

External docs: https://nodejs.org/api/fs.html#fspromisesstatpath-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object? |  | true |  |
| options.bigint | boolean? | false | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Stats> |  |

## [`writeFile(path, data, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L446)

External docs: https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromiseswritefilefile-data-options


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| FileHandle |  | false | filename or FileHandle |
| data | string \| Buffer \| Array \| DataView \| TypedArray |  | false |  |
| options | object? |  | true |  |
| options.encoding | string \| null | utf8 | true |  |
| options.mode | number | 0o666 | true |  |
| options.flag | string | w | true |  |
| options.signal | AbortSignal? |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


# [IPC](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L33)


 This is a low-level API that you don't need unless you are implementing
 a library on top of Socket SDK. A Socket SDK app has two or three processes.

 - The `Render` process, is the UI where the HTML, CSS, and JS are run.
 - The `Bridge` process, is the thin layer of code that manages everything.
 - The `Main` process, is for apps that need to run heavier compute jobs. And
   unlike electron it's optional.

 The Bridge process manages the Render and Main process, it may also broker
 data between them.

 The Binding process uses standard input and output as a way to communicate.
 Data written to the write-end of the pipe is buffered by the OS until it is
 read from the read-end of the pipe.

 The IPC protocol uses a simple URI-like scheme. Data is passed as
 ArrayBuffers.

 ```
 ipc://command?key1=value1&key2=value2...
 ```

 Example usage:
 ```js
 import { send } from 'socket:ipc'
 ```

## [`emit(name, value, target, options)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L1087)

Emit event to be dispatched on `window` object.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| value | any |  | false |  |
| target | EventTarget | window | true |  |
| options | Object |  | true |  |

## [`send(command, value, options)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L1146)

Sends an async IPC command request with parameters.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| command | string |  | false |  |
| value | any |  | true |  |
| options | object |  | true |  |
| options.cache | boolean | false | true |  |
| options.bytes | boolean | false | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Result> |  |


# [OS](https://github.com/socketsupply/socket/blob/master/api/os.js#L13)


 This module provides normalized system information from all the major
 operating systems.

 Example usage:
 ```js
 import { arch, platform } from 'socket:os'
 ```

## [`arch()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L56)

Returns the operating system CPU architecture for which Socket was compiled.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | 'arm64', 'ia32', 'x64', or 'unknown' |

## [`cpus()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L74)

External docs: https://nodejs.org/api/os.html#os_os_cpus
Returns an array of objects containing information about each CPU/core.
 The properties of the objects are:
 - model `<string>` - CPU model name.
 - speed `<number>` - CPU clock speed (in MHz).
 - times `<object>` - An object containing the fields user, nice, sys, idle, irq representing the number of milliseconds the CPU has spent in each mode.
   - user `<number>` - Time spent by this CPU or core in user mode.
   - nice `<number>` - Time spent by this CPU or core in user mode with low priority (nice).
   - sys `<number>` - Time spent by this CPU or core in system mode.
   - idle `<number>` - Time spent by this CPU or core in idle mode.
   - irq `<number>` - Time spent by this CPU or core in IRQ mode.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| cpus | Array<object> | An array of objects containing information about each CPU/core. |

## [`networkInterfaces()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L98)

External docs: https://nodejs.org/api/os.html#os_os_networkinterfaces
Returns an object containing network interfaces that have been assigned a network address.
 Each key on the returned object identifies a network interface. The associated value is an array of objects that each describe an assigned network address.
 The properties available on the assigned network address object include:
 - address `<string>` - The assigned IPv4 or IPv6 address.
 - netmask `<string>` - The IPv4 or IPv6 network mask.
 - family `<string>` - The address family ('IPv4' or 'IPv6').
 - mac `<string>` - The MAC address of the network interface.
 - internal `<boolean>` - Indicates whether the network interface is a loopback interface.
 - scopeid `<number>` - The numeric scope ID (only specified when family is 'IPv6').
 - cidr `<string>` - The CIDR notation of the interface.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | object | An object containing network interfaces that have been assigned a network address. |

## [`platform()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L186)

External docs: https://nodejs.org/api/os.html#os_os_platform
Returns the operating system platform.
 The returned value is equivalent to `process.platform`.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | 'android', 'cygwin', 'freebsd', 'linux', 'darwin', 'ios', 'openbsd', 'win32', or 'unknown' |

## [`type()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L195)

External docs: https://nodejs.org/api/os.html#os_os_type
Returns the operating system name.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | 'CYGWIN_NT', 'Mac', 'Darwin', 'FreeBSD', 'Linux', 'OpenBSD', 'Windows_NT', 'Win32', or 'Unknown' |

## [`isWindows()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L234)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | boolean | `true` if the operating system is Windows. |

## [`tmpdir()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L246)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | The operating system's default directory for temporary files. |

## [EOL](https://github.com/socketsupply/socket/blob/master/api/os.js#L294)

 The operating system's end-of-line marker. `'\r\n'` on Windows and `'\n'` on POSIX.

## [`rusage()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L306)

Get resource usage.

## [`uptime()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L316)

Returns the system uptime in seconds.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | The system uptime in seconds. |

## [`uname()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L327)

Returns the operating system name.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | The operating system name. |


# [Path](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L9)


 Example usage:
 ```js
 import { Path } from 'socket:path'
 ```

## [`resolve()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L41)

External docs: https://nodejs.org/api/path.html#path_path_resolve_paths
The path.resolve() method resolves a sequence of paths or path segments into an absolute path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| ...paths | strig |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`cwd(opts)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L71)

Computes current working directory for a path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | true |  |
| opts.posix Set to `true` to force POSIX style path | boolean |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`origin()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L95)

Computed location origin. Defaults to `socket:///` if not available.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`relative(options, from, to)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L106)

Computes the relative path from `from` to `to`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| from | PathComponent |  | false |  |
| to | PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`join(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L153)

Joins path components. This function may not return an absolute path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`dirname(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L206)

Computes directory name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`basename(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L242)

Computes base name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`extname(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L254)

Computes extension name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`normalize(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L265)

Computes normalized path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`format(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L315)

Formats `Path` object into a string.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | object \| Path |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

## [`parse(path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L331)

Parses input `path` into a `Path` instance.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | PathComponent |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | object |  |

## [Path](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L359)

A container for a parsed Path.

### [`from(input, cwd)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L381)

Creates a `Path` instance from `input` and optional `cwd`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| input | PathComponent |  | false |  |
| cwd | string |  | true |  |

### [`constructor(pathname, cwd)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L404)

`Path` class constructor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| pathname | string |  | false |  |
| cwd | string | Path.cwd() | true |  |

### [`isRelative()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L473)

`true` if the path is relative, otherwise `false.

### [`value()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L480)

The working value of this path.

### [`source()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L514)

The original source, unresolved.

### [`parent()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L522)

Computed parent path.

### [`root()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L541)

Computed root in path.

### [`dir()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L562)

Computed directory name in path.

### [`base()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L597)

Computed base name in path.

### [`name()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L609)

Computed base name in path without path extension.

### [`ext()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L617)

Computed extension name in path.

### [`drive()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L637)

The computed drive, if given in the path.

### [`toURL()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L644)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | URL |  |

### [`toString()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L652)

Converts this `Path` instance to a string.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


# [Process](https://github.com/socketsupply/socket/blob/master/api/process.js#L9)


 Example usage:
 ```js
 import process from 'socket:process'
 ```

## [`nextTick(callback)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L42)

Adds callback to the 'nextTick' queue.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| callback | Function |  | false |  |

## [`homedir()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L71)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | The home directory of the current user. |

## [`hrtime(time)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L80)

Computed high resolution time as a `BigInt`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| time | Array<number>? |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | bigint |  |

## [`exit(code)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L104)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| code | number | 0 | true | The exit code. Default: 0. |

## [`memoryUsage()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L116)

Returns an object describing the memory usage of the Node.js process measured in bytes.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Object |  |


# [Test](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L17)


 Provides a test runner for Socket Runtime.

 Example usage:
 ```js
 import { test } from 'socket:test'

 test('test name', async t => {
   t.equal(1, 1)
 })
 ```

## [`getDefaultTestRunnerTimeout()`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L54)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | The default timeout for tests in milliseconds. |

## [Test](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L69)



### [`constructor(name, fn, runner)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L76)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| fn | TestFn |  | false |  |
| runner | TestRunner |  | false |  |

### [`comment(msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L127)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| msg | string |  | false |  |

### [`plan(n)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L137)

Plan the number of assertions.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| n | number |  | false |  |

### [`deepEqual(actual, expected, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L148)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| actual | T |  | false |  |
| expected | T |  | false |  |
| msg | string |  | true |  |

### [`notDeepEqual(actual, expected, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L163)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| actual | T |  | false |  |
| expected | T |  | false |  |
| msg | string |  | true |  |

### [`equal(actual, expected, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L178)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| actual | T |  | false |  |
| expected | T |  | false |  |
| msg | string |  | true |  |

### [`notEqual(actual, expected, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L193)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| actual | unknown |  | false |  |
| expected | unknown |  | false |  |
| msg | string |  | true |  |

### [`fail(msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L206)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| msg | string |  | true |  |

### [`ok(actual, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L219)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| actual | unknown |  | false |  |
| msg | string |  | true |  |

### [`pass(msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L231)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| msg | string |  | true |  |

### [`ifError(err, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L240)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| err | Error \| null \| undefined |  | false |  |
| msg | string |  | true |  |

### [`throws(fn, expected, message)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L253)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fn | Function |  | false |  |
| expected | RegExp \| any |  | true |  |
| message | string |  | true |  |

### [`sleep(ms, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L302)

Sleep for ms with an optional msg


 ```js
 await t.sleep(100)
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| ms | number |  | false |  |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`requestAnimationFrame(msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L320)

Request animation frame with an optional msg. Falls back to a 0ms setTimeout when
 tests are run headlessly.


 ```js
 await t.requestAnimationFrame()
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`click(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L342)

Dispatch the `click`` method on an element specified by selector.


 ```js
 await t.click('.class button', 'Click a button')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`eventClick(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L364)

Dispatch the click window.MouseEvent on an element specified by selector.


 ```js
 await t.eventClick('.class button', 'Click a button with an event')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`dispatchEvent(event, target, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L392)

Dispatch an event on the target.


 ```js
 await t.dispatchEvent('my-event', '#my-div', 'Fire the my-event event')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string \| Event |  | false | The event name or Event instance to dispatch. |
| target | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element to dispatch the event on. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`focus(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L412)

Call the focus method on element specified by selector.


 ```js
 await t.focus('#my-div')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`blur(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L433)

Call the blur method on element specified by selector.


 ```js
 await t.blur('#my-div')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`type(selector, str, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L455)

Consecutively set the str value of the element specified by selector to simulate typing.


 ```js
 await t.typeValue('#my-div', 'Hello World', 'Type "Hello World" into #my-div')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| str | string |  | false | The string to type into the :focus element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`appendChild(parentSelector, el, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L487)

appendChild an element el to a parent selector element.


 ```js
 const myElement = createElement('div')
 await t.appendChild('#parent-selector', myElement, 'Append myElement into #parent-selector')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| parentSelector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element to appendChild on. |
| el | HTMLElement \| Element |  | false | A element to append to the parent element. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`removeElement(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L507)

Remove an element from the DOM.


 ```js
 await t.removeElement('#dom-selector', 'Remove #dom-selector')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element to remove from the DOM. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`elementVisible(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L526)

Test if an element is visible


 ```js
 await t.elementVisible('#dom-selector','Element is visible')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element to test visibility on. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`elementInvisible(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L547)

Test if an element is invisible


 ```js
 await t.elementInvisible('#dom-selector','Element is invisible')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element to test visibility on. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`waitFor(querySelectorOrFn, opts, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L571)

Test if an element is invisible


 ```js
 await t.waitFor('#dom-selector', { visible: true },'#dom-selector is on the page and visible')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| querySelectorOrFn | string \| (() => HTMLElement \| Element \| null \| undefined) |  | false | A query string or a function that returns an element. |
| opts | Object |  | true |  |
| opts.visible | boolean |  | true | The element needs to be visible. |
| opts.timeout | number |  | true | The maximum amount of time to wait. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<HTMLElement \| Element \| void> |  |

### [`waitForText(selector, opts, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L633)

Test if an element is invisible


 ```js
 await t.waitForText('#dom-selector', 'Text to wait for')
 ```

 ```js
 await t.waitForText('#dom-selector', /hello/i)
 ```

 ```js
 await t.waitForText('#dom-selector', {
   text: 'Text to wait for',
   multipleTags: true
 })
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| HTMLElement \| Element |  | false | A CSS selector string, or an instance of HTMLElement, or Element. |
| opts | WaitForTextOpts \| string \| RegExp |  | true |  |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<HTMLElement \| Element \| void> |  |

### [`querySelector(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L670)

Run a querySelector as an assert and also get the results


 ```js
 const element = await t.querySelector('#dom-selector')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string |  | false | A CSS selector string, or an instance of HTMLElement, or Element to select. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | HTMLElement \| Element |  |

### [`querySelectorAll(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L689)

Run a querySelectorAll as an assert and also get the results


 ```js
 const elements = await t.querySelectorAll('#dom-selector', '')
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string |  | false | A CSS selector string, or an instance of HTMLElement, or Element to select. |
| msg | string |  | true |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Array<HTMLElement \| Element> |  |

### [`getComputedStyle(selector, msg)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L718)

Retrieves the computed styles for a given element.


 ```js
 // Using CSS selector
 const style = getComputedStyle('.my-element', 'Custom success message');
 ```

 ```js
 // Using Element object
 const el = document.querySelector('.my-element');
 const style = getComputedStyle(el);
 ```

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| selector | string \| Element |  | false | The CSS selector or the Element object for which to get the computed styles. |
| msg | string |  | true | An optional message to display when the operation is successful. Default message will be generated based on the type of selector. |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | CSSStyleDeclaration | The computed styles of the element. |

### [`run()`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L815)

   pass: number,
   fail: number
 }>}

## [TestRunner](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L896)



### [`constructor(report)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L901)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| report | (lines: string) => void |  | true |  |

### [`nextId()`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L953)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |

### [`add(name, fn, only)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L963)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| fn | TestFn |  | false |  |
| only | boolean |  | false |  |

### [`run()`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L985)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |

### [`onFinish())`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L1032)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| ) | (result: { total: number, success: number, fail: number  | > void} callback | false |  |

## [`only(name, fn)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L1060)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| fn | TestFn |  | true |  |

## [`skip(_name, _fn)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L1070)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| _name | string |  | false |  |
| _fn | TestFn |  | true |  |

## [`setStrict(strict)`](https://github.com/socketsupply/socket/blob/master/api/test/index.js#L1076)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| strict | boolean |  | false |  |



# [Window](https://github.com/socketsupply/socket/blob/master/api/window.js#L11)

External docs: module:Application Application

 Provides ApplicationWindow class and methods

 Usaully you don't need to use this module directly, instance of ApplicationWindow
 are returned by the methods of the {@link module:Application Application} module.

## [ApplicationWindow](https://github.com/socketsupply/socket/blob/master/api/window.js#L31)

 Represents a window in the application

### [`index()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L59)

Get the index of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | the index of the window |

### [`getSize()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L67)

Get the size of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | { width: number, height: number  | } - the size of the window |

### [`getTitle()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L78)

Get the title of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | the title of the window |

### [`getStatus()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L86)

Get the status of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | the status of the window |

### [`close()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L94)

Close the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> | the options of the window |

### [`show()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L109)

Shows the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`hide()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L118)

Hides the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`setTitle(title)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L128)

Sets the title of the window

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| title | string |  | false | the title of the window |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`setSize(opts)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L141)

Sets the size of the window

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.width | number \| string |  | true | the width of the window |
| opts.height | number \| string |  | true | the height of the window |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`navigate(path)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L181)

Navigate the window to a given path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | object |  | false | file path |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`showInspector()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L190)

Opens the Web Inspector for the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> |  |

### [`setBackgroundColor(opts)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L207)

Sets the background color of the window

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.red | number |  | false | the red value |
| opts.green | number |  | false | the green value |
| opts.blue | number |  | false | the blue value |
| opts.alpha | number |  | false | the alpha value |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> |  |

### [`setContextMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L217)

Opens a native context menu.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> |  |

### [`showOpenFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L234)

Shows a native open file dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |

### [`showSaveFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L245)

Shows a native save file dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |

### [`showDirectoryFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L256)

Shows a native directory dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |

### [`send(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L271)

Sends an IPC message to the window or to qthe backend.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |
| options.window | number |  | true | the window to send the message to |
| options.backend | boolean | false | true | whether to send the message to the backend |
| options.event | string |  | false | the event to send |
| options.value | string \| object |  | true | the value to send |

### [`openExternal(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L308)

Opens an URL in the default browser.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |

### [`addListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L319)

Adds a listener to the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |

### [`on(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L337)

Adds a listener to the window. An alias for `addListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |

### [`once(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L354)

Adds a listener to the window. The listener is removed after the first call.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |

### [`removeListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L370)

Removes a listener from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |

### [`removeAllListeners(event)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L383)

Removes all listeners from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listeners from |

### [`off(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L399)

Removes a listener from the window. An alias for `removeListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |

