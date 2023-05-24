
# [Application](https://github.com/socketsupply/socket/blob/master/api/application.js#L13)


 Provides Application level methods

 Example usage:
 ```js
 import { createWindow } from 'socket:application'
 ```

## [`createWindow(opts)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L40)

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


## [`getScreenSize()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L98)

Returns the current screen size.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`getWindows(indices)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L118)

Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| indices | number \| undefined |  | false | the indices of the windows |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Object.<number, ApplicationWindow>> |  |


## [`getWindow(index)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L137)

Returns the ApplicationWindow instance for the given index

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| index | number |  | false | the index of the window |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ApplicationWindow> | the ApplicationWindow instance or null if the window does not exist |


## [`getCurrentWindow()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L147)

Returns the ApplicationWindow instance for the current window.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ApplicationWindow> |  |


## [`exit(code)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L156)

Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| code | object |  | false | an exit code |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`setSystemMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L253)

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


## [`setSystemMenuItemEnabled(value)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L307)

Set the enabled state of the system menu.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| value | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [runtimeVersion](https://github.com/socketsupply/socket/blob/master/api/application.js#L315)

Socket Runtime version.

## [debug](https://github.com/socketsupply/socket/blob/master/api/application.js#L321)

Runtime debug flag.

## [config](https://github.com/socketsupply/socket/blob/master/api/application.js#L327)

Application configuration.

## [backend](https://github.com/socketsupply/socket/blob/master/api/application.js#L332)

The application's backend instance.

### [`open(opts)`](https://github.com/socketsupply/socket/blob/master/api/application.js#L338)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.force | boolean | false | false | whether to force the existing process to close |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`close()`](https://github.com/socketsupply/socket/blob/master/api/application.js#L346)



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


### [`subscribe(id )`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L119)

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
| id | string |  | false | A well-known UUID |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`publish(id, value)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L142)

Start advertising a new value for a well-known UUID

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| id | string |  | false | A well-known UUID |
| value | string |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


# [Crypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L14)


 Some high-level methods around the `crypto.subtle` API for getting
 random bytes and hashing.

 Example usage:
 ```js
 import { randomBytes } from 'socket:crypto'
 ```

## [webcrypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L23)

WebCrypto API

## [ready](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L52)

A promise that resolves when all internals to be loaded/ready.

## [`getRandomValues(buffer)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L67)

External docs: https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues

Generate cryptographically strong random values into the `buffer`

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| buffer | TypedArray |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | TypedArray |  |


## [`rand64()`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L86)

This is a `FunctionDeclaration` named `rand64` in `api/crypto.js`, it's exported but undocumented.


## [RANDOM_BYTES_QUOTA](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L94)

Maximum total size of random bytes per page

## [MAX_RANDOM_BYTES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L99)

Maximum total size for random bytes.

## [MAX_RANDOM_BYTES_PAGES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L104)

Maximum total amount of allocated per page of bytes (max/quota)

## [`randomBytes(size)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L112)

Generate `size` random bytes.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The number of bytes to generate. The size must not be larger than 2**31 - 1. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Buffer | A promise that resolves with an instance of socket.Buffer with random bytes. |


## [`createDigest(algorithm, message)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L139)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| algorithm | string |  | false | `SHA-1` | `SHA-256` | `SHA-384` | `SHA-512` |
| message | Buffer \| TypedArray \| DataView |  | false | An instance of socket.Buffer, TypedArray or Dataview. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Buffer> | A promise that resolves with an instance of socket.Buffer with the hash. |


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


## [`ERR_SOCKET_ALREADY_BOUND` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1172)

Thrown when a socket is already bound.

## [`ERR_SOCKET_DGRAM_IS_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1189)

Thrown when the socket is already connected.

## [`ERR_SOCKET_DGRAM_NOT_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1196)

Thrown when the socket is not connected.

## [`ERR_SOCKET_DGRAM_NOT_RUNNING` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1204)

Thrown when the socket is not running (not bound or connected).

## [`ERR_SOCKET_BAD_TYPE` (extends `TypeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1211)

Thrown when a bad socket type is used in an argument.

## [`ERR_SOCKET_BAD_PORT` (extends `RangeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1221)

Thrown when a bad port is given.

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

## [`lookup(hostname, opts, cb)`](https://github.com/socketsupply/socket/blob/master/api/dns/index.js#L60)

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
| opts | Object |  | true | An options object. |
| opts.family | number \| string | 0 | false | The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0. |
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
| opts.family | number \| string | 0 | false | The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise |  |


## [Event](https://github.com/socketsupply/socket/blob/master/api/events.js#L472)

This is a `VariableDeclaration` named `Event` in `api/events.js`, it's exported but undocumented.


## [CustomEvent](https://github.com/socketsupply/socket/blob/master/api/events.js#L474)

This is a `VariableDeclaration` named `CustomEvent` in `api/events.js`, it's exported but undocumented.


## [MessageEvent](https://github.com/socketsupply/socket/blob/master/api/events.js#L489)

This is a `VariableDeclaration` named `MessageEvent` in `api/events.js`, it's exported but undocumented.


## [ErrorEvent](https://github.com/socketsupply/socket/blob/master/api/events.js#L513)

This is a `VariableDeclaration` named `ErrorEvent` in `api/events.js`, it's exported but undocumented.


# [File System](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L26)


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

## [`access(path, mode , callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L81)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsopenpath-flags-mode-callback

Asynchronously check access a file for a given mode calling `callback`
 upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string | F_OK(0) | true |  |
| callback | function(err, fd) |  | false |  |


## [`chmod(path, mode, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L114)

External docs: https://nodejs.org/api/fs.html#fschmodpath-mode-callback

Asynchronously changes the permissions of a file.
 No arguments other than a possible exception are given to the completion callback



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |
| callback | function(err) |  | false |  |


## [`close(fd, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L144)

Asynchronously close a file descriptor calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| callback | function(err) |  | true |  |


## [`copyFile()`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L160)

This is a `FunctionDeclaration` named `copyFile` in `api/fs/index.js`, it's exported but undocumented.


## [`createReadStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L169)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | fs.ReadStream |  |


## [`createWriteStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L209)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | fs.WriteStream |  |


## [`fstat(fd, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L253)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsfstatfd-options-callback

Invokes the callback with the <fs.Stats> for the file descriptor. See
 the POSIX fstat(2) documentation for more detail.



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false | A file descriptor. |
| options | Object |  | true | An options object. |
| callback | function |  | false | The function to call after completion. |


## [`open(path, flags , mode , callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L336)

Asynchronously open a file calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| flags | string | r | true |  |
| mode | string | 0o666 | true |  |
| callback | function(err, fd) |  | false |  |


## [`opendir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L389)

Asynchronously open a directory calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | Object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |
| callback | function(err, fd) |  | false |  |


## [`read(fd, buffer, offset, length, position, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L415)

Asynchronously read from an open file descriptor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| buffer | object \| Buffer \| TypedArray |  | false | The buffer that the data will be written to. |
| offset | number |  | false | The position in buffer to write the data to. |
| length | number |  | false | The number of bytes to read. |
| position | number \| BigInt \| null |  | false | Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged. |
| callback | function(err, bytesRead, buffer) |  | false |  |


## [`readdir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L449)

Asynchronously read all entries in a directory.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL  |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |
| callback | function(err, buffer) |  | false |  |


## [`readFile(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L500)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.flag | string | r | true |  |
| options.signal | AbortSignal |  | true |  |
| callback | function(err, buffer) |  | false |  |


## [`stat(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L573)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false | filename or file descriptor |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.flag | string | r | true |  |
| options.signal | AbortSignal |  | true |  |
| callback | function(err, data) |  | false |  |


## [`writeFile(path, data, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L643)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false | filename or file descriptor |
| data | string \| Buffer \| TypedArray \| DataView \| object  |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.mode | string | 0o666 | true |  |
| options.flag | string | w | true |  |
| options.signal | AbortSignal |  | true |  |
| callback | function(err) |  | false |  |


## [`writev()`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L680)

This is a `FunctionDeclaration` named `writev` in `api/fs/index.js`, it's exported but undocumented.


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

## [`access(path, mode, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L62)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesaccesspath-mode

Asynchronously check access a file.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string |  | true |  |
| options | object |  | true |  |


## [`chmod(path, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L79)

External docs: https://nodejs.org/api/fs.html#fspromiseschmodpath-mode



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


## [`mkdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L148)

Asynchronously creates a directory.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | String |  | false | The path to create |
| options | Object |  | false | The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Primise<any> | Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true. |


## [`open(path, flags, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L172)

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


## [`opendir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L184)

External docs: https://nodejs.org/api/fs.html#fspromisesopendirpath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.bufferSize | number | 32 | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<FileSystem,Dir> |  |


## [`readdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L196)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesreaddirpath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |


## [`readFile(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L229)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesreadfilepath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string |  | false |  |
| options | object |  | true |  |
| options.encoding | string \| null | null | true |  |
| options.flag | string | r | true |  |
| options.signal | AbortSignal |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Buffer \| string> |  |


## [`stat(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L284)

External docs: https://nodejs.org/api/fs.html#fspromisesstatpath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |
| options.bigint | boolean | false | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Stats> |  |


## [`writeFile(path, data, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L337)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromiseswritefilefile-data-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| FileHandle |  | false | filename or FileHandle |
| data | string \| Buffer \| Array \| DataView \| TypedArray \| Stream |  | false |  |
| options | object |  | true |  |
| options.encoding | string \| null | utf8 | false |  |
| options.mode | number | 0o666 | false |  |
| options.flag | string | w | false |  |
| options.signal | AbortSignal |  | true |  |


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

## [`emit(name, value, target , options)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L1027)

Emit event to be dispatched on `window` object.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| value | any |  | false |  |
| target | EventTarget | window | true |  |
| options | Object |  | true |  |


## [`send(command, value, options)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L1086)

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

## [`arch()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L52)

This is a `FunctionDeclaration` named `arch` in `api/os.js`, it's exported but undocumented.


## [`cpus()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L56)

This is a `FunctionDeclaration` named `cpus` in `api/os.js`, it's exported but undocumented.


## [`networkInterfaces()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L66)

This is a `FunctionDeclaration` named `networkInterfaces` in `api/os.js`, it's exported but undocumented.


## [`platform()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L148)

This is a `FunctionDeclaration` named `platform` in `api/os.js`, it's exported but undocumented.


## [`type()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L152)

This is a `FunctionDeclaration` named `type` in `api/os.js`, it's exported but undocumented.


## [`isWindows()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L189)

This is a `FunctionDeclaration` named `isWindows` in `api/os.js`, it's exported but undocumented.


## [`tmpdir()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L198)

This is a `FunctionDeclaration` named `tmpdir` in `api/os.js`, it's exported but undocumented.


## [EOL](https://github.com/socketsupply/socket/blob/master/api/os.js#L242)

This is a `VariableDeclaration` named `EOL` in `api/os.js`, it's exported but undocumented.


## [`rusage()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L250)

This is a `FunctionDeclaration` named `rusage` in `api/os.js`, it's exported but undocumented.


## [`uptime()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L256)

This is a `FunctionDeclaration` named `uptime` in `api/os.js`, it's exported but undocumented.


## [`uname()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L262)

This is a `FunctionDeclaration` named `uname` in `api/os.js`, it's exported but undocumented.


## [`hrtime()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L272)

This is a `FunctionDeclaration` named `hrtime` in `api/os.js`, it's exported but undocumented.


## [`availableMemory()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L281)

This is a `FunctionDeclaration` named `availableMemory` in `api/os.js`, it's exported but undocumented.


# [Peer](https://github.com/socketsupply/socket/blob/master/api/peer.js#L14)

External docs: https://socketsupply.co/guides/#p2p-guide


 Provides a higher level API over the stream-relay protocol.

 Example usage:
 ```js
 import { Peer } from 'socket:peer'
 ```


## [`Peer` (extends `EventEmitter`)](https://github.com/socketsupply/socket/blob/master/api/peer.js#L52)


The Peer class is an EventEmitter. It emits events when new network events
are received (.on), it can also emit new events to the network (.emit).

```js
import { Peer } from 'socket:peer'

const pair = await Peer.createKeys()
const clusterId = await Peer.createClusterId()

const peer = new Peer({ ...pair, clusterId })

peer.on('greeting', (value, peer, address, port) => {
  console.log(value)
})

window.onload = () => {
  const value = { english: 'hello, world' }
  const packet = await peer.emit('greeting', value)
}
```

### [`constructor(options)`](https://github.com/socketsupply/socket/blob/master/api/peer.js#L62)

`Peer` class constructor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | Object |  | false | All options for the Peer constructor |
| options.publicKey | string |  | false | The public key required to sign and read |
| options.privateKey | string |  | false | The private key required to sign and read |
| options.clusterId | string |  | false | A unique appliction identity |
| options.scheme | string |  | false | Specify which encryption scheme to use (ie, 'PTP') |
| options.peers | Array |  | false | An array of RemotePeer |


### [`createKeys()`](https://github.com/socketsupply/socket/blob/master/api/peer.js#L82)

A method that will generate a public and private key pair.
 The ed25519 pair can be stored by an app with a secure API.


| Return Value | Type | Description |
| :---         | :--- | :---        |
| pair | Object<Pair> | A pair of keys |


### [`createClusterId()`](https://github.com/socketsupply/socket/blob/master/api/peer.js#L96)

Create a clusterId from random bytes

| Return Value | Type | Description |
| :---         | :--- | :---        |
| id | string | a hex encoded sha256 hash |


### [`emit(event, message)`](https://github.com/socketsupply/socket/blob/master/api/peer.js#L107)

Emits a message to the network


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | The name of the event to emit to the network |
| message | Buffer |  | false | The data to emit to the network |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Object<Packet> | The packet that will be sent when possible |


### [`join()`](https://github.com/socketsupply/socket/blob/master/api/peer.js#L130)

Starts the process of connecting to the network.


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Peer | Returns an instance of the underlying network peer |


# [Path](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L9)


 Example usage:
 ```js
 import { Path } from 'socket:path'
 ```

## [`Path` (extends `URL`)](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L32)

A container for a parsed Path.

### [`cwd(opts)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L39)

Computes current working directory for a path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | true |  |
| opts.posix Set to `true` to force POSIX style path | boolean |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`resolve(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L58)

Resolves path components to an absolute path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`relative(options, from, to)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L86)

Computes the relative path from `from` to `to`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| from | PathComponent |  | false |  |
| to | PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`join(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L133)

Joins path components. This function may not return an absolute path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`dirname(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L163)

Computes directory name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`basename(options, components)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L235)

Computes base name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| components | ...PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`extname(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L250)

Computes extension name of path.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`normalize(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L261)

Computes normalized path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`format(options, path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L311)

Formats `Path` object into a string.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| path | object \| Path |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


### [`parse(path)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L327)

Parses input `path` into a `Path` instance.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | PathComponent |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | object |  |


### [`from(input, cwd)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L353)

Creates a `Path` instance from `input` and optional `cwd`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| input | PathComponent |  | false |  |
| cwd | string |  | false |  |


### [`constructor(pathname, cwd )`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L374)

`Path` class constructor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| pathname | string |  | false |  |
| cwd | string | Path.cwd() | false |  |


### [`isRelative()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L407)

`true` if the path is relative, otherwise `false.

### [`value()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L414)

The working value of this path.

### [`source()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L425)

The original source, unresolved.

### [`parent()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L433)

Computed parent path.

### [`root()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L451)

Computed root in path.

### [`dir()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L472)

Computed directory name in path.

### [`base()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L507)

Computed base name in path.

### [`name()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L519)

Computed base name in path without path extension.

### [`ext()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L527)

Computed extension name in path.

### [`drive()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L545)

The computed drive, if given in the path.

### [`toURL()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L552)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | URL |  |


### [`toString()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L560)

Converts this `Path` instance to a string.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string |  |


# [Process](https://github.com/socketsupply/socket/blob/master/api/process.js#L9)


 Example usage:
 ```js
 import process from 'socket:process'
 ```

## [`nextTick()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L38)

This is a `FunctionDeclaration` named `nextTick` in `api/process.js`, it's exported but undocumented.


## [`homedir()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L67)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | The home directory of the current user. |


## [`hrtime(time)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L76)

Computed high resolution time as a `BigInt`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| time | Array<number>? |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | bigint |  |


## [`exit(code)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L100)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| code | number | 0 | true | The exit code. Default: 0. |


## [`memoryUsage()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L108)

This is a `FunctionDeclaration` named `memoryUsage` in `api/process.js`, it's exported but undocumented.


# [Window](https://github.com/socketsupply/socket/blob/master/api/window.js#L11)

External docs: module:Application Application


 Provides ApplicationWindow class and methods

 Usaully you don't need to use this module directly, instance of ApplicationWindow
 are returned by the methods of the {@link module:Application Application} module.

## [ApplicationWindow](https://github.com/socketsupply/socket/blob/master/api/window.js#L28)

 Represents a window in the application

### [`index()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L56)

Get the index of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | the index of the window |


### [`getSize()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L64)

Get the size of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | { width: number, height: number  | } - the size of the window |


### [`getTitle()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L75)

Get the title of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | the title of the window |


### [`getStatus()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L83)

Get the status of the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | the status of the window |


### [`close()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L91)

Close the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> | the options of the window |


### [`show()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L106)

Shows the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`hide()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L115)

Hides the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`setTitle(title)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L125)

Sets the title of the window

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| title | string |  | false | the title of the window |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`setSize(opts)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L138)

Sets the size of the window

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.width | number \| string |  | true | the width of the window |
| opts.height | number \| string |  | true | the height of the window |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`navigate(path)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L178)

Navigate the window to a given path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | object |  | false | file path |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`showInspector()`](https://github.com/socketsupply/socket/blob/master/api/window.js#L187)

Opens the Web Inspector for the window

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> |  |


### [`setBackgroundColor(opts)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L204)

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


### [`setContextMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L214)

Opens a native context menu.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<object> |  |


### [`showOpenFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L231)

Shows a native open file dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |


### [`showSaveFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L242)

Shows a native save file dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |


### [`showDirectoryFilePicker(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L253)

Shows a native directory dialog.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string[]> | an array of file paths |


### [`send(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L268)

Sends an IPC message to the window or to qthe backend.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |
| options.window | number |  | true | the window to send the message to |
| options.backend | boolean | false | true | whether to send the message to the backend |
| options.event | string |  | false | the event to send |
| options.value | string \| object |  | true | the value to send |


### [`openExternal(options)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L305)

Opens an URL in the default browser.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`addListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L316)

Adds a listener to the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`on(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L334)

Adds a listener to the window. An alias for `addListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`once(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L351)

Adds a listener to the window. The listener is removed after the first call.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to listen to |
| cb | function(*): void |  | false | the callback to call |


### [`removeListener(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L367)

Removes a listener from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |


### [`removeAllListeners(event)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L380)

Removes all listeners from the window.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listeners from |


### [`off(event, cb)`](https://github.com/socketsupply/socket/blob/master/api/window.js#L396)

Removes a listener from the window. An alias for `removeListener`.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| event | string |  | false | the event to remove the listener from |
| cb | function(*): void |  | false | the callback to remove |


## [constants](https://github.com/socketsupply/socket/blob/master/api/window.js#L406)

This is a `VariableDeclaration` named `constants` in `api/window.js`, it's exported but undocumented.

