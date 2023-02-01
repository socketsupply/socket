
# [Backend](https://github.com/socketsupply/socket/blob/master/api/backend.js#L7)


 Provides methods for the backend process management

## [`open(opts)`](https://github.com/socketsupply/socket/blob/master/api/backend.js#L14)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.force | boolean | false | false | whether to force existing process to close |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`close()`](https://github.com/socketsupply/socket/blob/master/api/backend.js#L22)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`sendToProcess()`](https://github.com/socketsupply/socket/blob/master/api/backend.js#L29)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


# [Bluetooth](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L7)


 A high level, cross-platform API for Bluetooth Pub-Sub

## [`Bluetooth` (extends `EventEmitter`)](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L15)

Create an instance of a Bluetooth service.

### [`constructor(serviceId)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L23)

constructor is an example property that is set to `true`
 Creates a new service with key-value pairs

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| serviceId | string |  | false | Given a default value to determine the type |


### [`start()`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L60)

Start the bluetooth service.

| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


### [`subscribe(id )`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L80)

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


### [`publish(id, value)`](https://github.com/socketsupply/socket/blob/master/api/bluetooth.js#L93)

Start advertising a new value for a well-known UUID

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| id | string |  | false | A well-known UUID |
| value | string |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


### [`undefined(buf, hashAlgorithm)`](https://github.com/socketsupply/socket/blob/master/api/bootstrap.js#L25)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| buf | Buffer \| String |  | false |  |
| hashAlgorithm | string |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<string> |  |


## [`checkHash(dest, hash, hashAlgorithm)`](https://github.com/socketsupply/socket/blob/master/api/bootstrap.js#L36)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| dest | string |  | false | file path |
| hash | string |  | false | hash string |
| hashAlgorithm | string |  | false | hash algorithm |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<boolean> |  |


### [`write(options)`](https://github.com/socketsupply/socket/blob/master/api/bootstrap.js#L74)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false |  |
| options.fileBuffer | Uint8Array |  | false |  |
| options.dest | string |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


### [`download(url)`](https://github.com/socketsupply/socket/blob/master/api/bootstrap.js#L104)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| url | string |  | false | url to download |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Uint8Array> |  |


## [`bootstrap()`](https://github.com/socketsupply/socket/blob/master/api/bootstrap.js#L131)

This is a `FunctionDeclaration` named `bootstrap` in `api/bootstrap.js`, it's exported but undocumented.


# [Crypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L8)


 Some high level methods around the `crypto.subtle` api for getting
 random bytes and hashing.

## [webcrypto](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L25)

WebCrypto API

## [`getRandomValues(buffer)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L33)

External docs: https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues

Generate cryptographically strong random values into `buffer`

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| buffer | TypedArray |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | TypedArray |  |


## [`rand64()`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L45)

This is a `FunctionDeclaration` named `rand64` in `api/crypto.js`, it's exported but undocumented.


## [RANDOM_BYTES_QUOTA](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L53)

Maximum total size of random bytes per page

## [MAX_RANDOM_BYTES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L58)

Maximum total size for random bytes.

## [MAX_RANDOM_BYTES_PAGES](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L63)

Maximum total amount of allocated per page of bytes (max/quota)

## [`randomBytes(size)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L71)

Generate `size` random bytes.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The number of bytes to generate. The size must not be larger than 2**31 - 1. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Buffer | A promise that resolves with an instance of socket.Buffer with random bytes. |


## [`createDigest(algorithm, message)`](https://github.com/socketsupply/socket/blob/master/api/crypto.js#L98)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| algorithm | string |  | false | `SHA-1` | `SHA-256` | `SHA-384` | `SHA-512` |
| message | Buffer \| TypedArray \| DataView |  | false | An instance of socket.Buffer, TypedArray or Dataview. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Buffer> | A promise that resolves with an instance of socket.Buffer with the hash. |


# [Dgram](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L8)


 This module provides an implementation of UDP datagram sockets. It does
 not (yet) provide any of the multicast methods or properties.

## [`ERR_SOCKET_ALREADY_BOUND` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L44)

Thrown when a socket is already bound.

## [`ERR_SOCKET_DGRAM_IS_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L61)

Thrown when the socket is already connected.

## [`ERR_SOCKET_DGRAM_NOT_CONNECTED` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L68)

Thrown when the socket is not connected.

## [`ERR_SOCKET_DGRAM_NOT_RUNNING` (extends `SocketError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L76)

Thrown when the socket is not running (not bound or connected).

## [`ERR_SOCKET_BAD_TYPE` (extends `TypeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L83)

Thrown when a bad socket type is used in an argument.

## [`ERR_SOCKET_BAD_PORT` (extends `RangeError`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L93)

Thrown when a bad port is given.

## [`createSocket(options, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L632)

Creates a `Socket` instance.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | string \| Object |  | false | either a string ('udp4' or 'udp6') or an options object |
| options.type | string |  | true | The family of socket. Must be either 'udp4' or 'udp6'. Required. |
| options.reuseAddr | boolean | false | true | When true socket.bind() will reuse the address, even if another process has already bound a socket on it. Default: false. |
| options.ipv6Only | boolean | false | true | Setting ipv6Only to true will disable dual-stack support, i.e., binding to address :: won't make 0.0.0.0 be bound. Default: false. |
| options.recvBufferSize | number |  | true | Sets the SO_RCVBUF socket value. |
| options.sendBufferSize | number |  | true | Sets the SO_SNDBUF socket value. |
| options.signal | AbortSignal |  | true | An AbortSignal that may be used to close a socket. |
| callback | function |  | true | Attached as a listener for 'message' events. Optional. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Socket |  |


## [`Socket` (extends `EventEmitter`)](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L638)

New instances of dgram.Socket are created using dgram.createSocket().
 The new keyword is not to be used to create dgram.Socket instances.

### [`bind(port, address, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L709)

External docs: https://nodejs.org/api/dgram.html#socketbindport-address-callback

Listen for datagram messages on a named port and optional address
 If address is not specified, the operating system will attempt to
 listen on all addresses. Once binding is complete, a 'listening'
 event is emitted and the optional callback function is called.

 If binding fails, an 'error' event is emitted.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| port | number |  | false | The port to to listen for messages on |
| address | string |  | false | The address to bind to (0.0.0.0) |
| callback | function |  | false | With no parameters. Called when binding is complete. |


### [`connect(port, host, connectListener)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L762)

External docs: https://nodejs.org/api/dgram.html#socketconnectport-address-callback

Associates the dgram.Socket to a remote address and port. Every message sent
 by this handle is automatically sent to that destination. Also, the socket
 will only receive messages from that remote peer. Trying to call connect()
 on an already connected socket will result in an ERR_SOCKET_DGRAM_IS_CONNECTED
 exception. If address is not provided, '127.0.0.1' (for udp4 sockets) or '::1'
 (for udp6 sockets) will be used by default. Once the connection is complete,
 a 'connect' event is emitted and the optional callback function is called.
 In case of failure, the callback is called or, failing this, an 'error' event
 is emitted.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| port | number |  | false | Port the client should connect to. |
| host | string |  | true | Host the client should connect to. |
| connectListener | function |  | true | Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once. |


### [`disconnect()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L797)

External docs: https://nodejs.org/api/dgram.html#socketdisconnect

A synchronous function that disassociates a connected dgram.Socket from
 its remote address. Trying to call disconnect() on an unbound or already
 disconnected socket will result in an ERR_SOCKET_DGRAM_NOT_CONNECTED exception.


### [`send(msg, offset, length, port, address, callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L856)

External docs: https://nodejs.org/api/dgram.html#socketsendmsg-offset-length-port-address-callback

Broadcasts a datagram on the socket. For connectionless sockets, the
 destination port and address must be specified. Connected sockets, on the
 other hand, will use their associated remote endpoint, so the port and
 address arguments must not be set.

 > The msg argument contains the message to be sent. Depending on its type,
 different behavior can apply. If msg is a Buffer, any TypedArray or a
 DataView, the offset and length specify the offset within the Buffer where
 the message begins and the number of bytes in the message, respectively.
 If msg is a String, then it is automatically converted to a Buffer with
 'utf8' encoding. With messages that contain multi-byte characters, offset
 and length will be calculated with respect to byte length and not the
 character position. If msg is an array, offset and length must not be
 specified.

 > The address argument is a string. If the value of address is a host name,
 DNS will be used to resolve the address of the host. If address is not
 provided or otherwise nullish, '127.0.0.1' (for udp4 sockets) or '::1'
 (for udp6 sockets) will be used by default.

 > If the socket has not been previously bound with a call to bind, the socket
 is assigned a random port number and is bound to the "all interfaces"
 address ('0.0.0.0' for udp4 sockets, '::0' for udp6 sockets.)

 > An optional callback function may be specified to as a way of reporting DNS
 errors or for determining when it is safe to reuse the buf object. DNS
 lookups delay the time to send for at least one tick of the Node.js event
 loop.

 > The only way to know for sure that the datagram has been sent is by using a
 callback. If an error occurs and a callback is given, the error will be
 passed as the first argument to the callback. If a callback is not given,
 the error is emitted as an 'error' event on the socket object.

 > Offset and length are optional but both must be set if either are used.
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


### [`close(callback)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L936)

External docs: https://nodejs.org/api/dgram.html#socketclosecallback

Close the underlying socket and stop listening for data on it. If a
 callback is provided, it is added as a listener for the 'close' event.



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| callback | function |  | true | Called when the connection is completed or on error. |


### [`address()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L994)

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


### [`remoteAddress()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1029)

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


### [`setRecvBufferSize(size)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1060)

External docs: https://nodejs.org/api/dgram.html#socketsetrecvbuffersizesize

Sets the SO_RCVBUF socket option. Sets the maximum socket receive buffer in
 bytes.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The size of the new receive buffer |


### [`setSendBufferSize(size)`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1077)

External docs: https://nodejs.org/api/dgram.html#socketsetsendbuffersizesize

Sets the SO_SNDBUF socket option. Sets the maximum socket send buffer in
 bytes.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| size | number |  | false | The size of the new send buffer |


### [`getRecvBufferSize()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1090)

External docs: https://nodejs.org/api/dgram.html#socketgetrecvbuffersize



### [`getSendBufferSize()`](https://github.com/socketsupply/socket/blob/master/api/dgram.js#L1098)

External docs: https://nodejs.org/api/dgram.html#socketgetsendbuffersize



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | number | the SO_SNDBUF socket send buffer size in bytes. |


# [DNS](https://github.com/socketsupply/socket/blob/master/api/dns/index.js#L13)


 This module enables name resolution. For example, use it to look up IP
 addresses of host names. Although named for the Domain Name System (DNS),
 it does not always use the DNS protocol for lookups. dns.lookup() uses the
 operating system facilities to perform name resolution. It may not need to
 perform any network communication. To perform name resolution the way other
 applications on the same system do, use dns.lookup().

## [`lookup(hostname, opts, cb)`](https://github.com/socketsupply/socket/blob/master/api/dns/index.js#L48)

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


# [DNS.promises](https://github.com/socketsupply/socket/blob/master/api/dns/promises.js#L13)


 This module enables name resolution. For example, use it to look up IP
 addresses of host names. Although named for the Domain Name System (DNS),
 it does not always use the DNS protocol for lookups. dns.lookup() uses the
 operating system facilities to perform name resolution. It may not need to
 perform any network communication. To perform name resolution the way other
 applications on the same system do, use dns.lookup().

## [`lookup(hostname, opts)`](https://github.com/socketsupply/socket/blob/master/api/dns/promises.js#L23)

External docs: https://nodejs.org/api/dns.html#dnspromiseslookuphostname-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| hostname | string |  | false | The host name to resolve. |
| opts | Object |  | true | An options object. |
| opts.family | number \| string | 0 | false | The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise |  |


# [File System](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L24)


 This module enables interacting with the file system in a way modeled on
 standard POSIX functions.

 The Application Sandbox restricts access to the file system.
 Please see the Application Sandbox documentation for more information:
 https://sockets.sh/guides/#working-with-the-file-system-on-ios

 To use the promise-based APIs:

 ```js
 import * as fs from 'socket:fs/promises';
 ```

 To use the callback and async APIs:

 ```js
 import * as fs from 'socket:fs';
 ```

## [`access(path, mode , callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L79)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsopenpath-flags-mode-callback

Asynchronously check access a file for a given mode calling `callback`
 upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string | F_OK(0) | true |  |
| callback | function(err, fd) |  | false |  |


## [`chmod(path, mode, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L112)

External docs: https://nodejs.org/api/fs.html#fschmodpath-mode-callback

Asynchronously changes the permissions of a file.
 No arguments other than a possible exception are given to the completion callback



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |
| callback | function(err) |  | false |  |


## [`close(fd, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L142)

Asynchronously close a file descriptor calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| callback | function(err) |  | true |  |


## [`copyFile()`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L158)

This is a `FunctionDeclaration` named `copyFile` in `api/fs/index.js`, it's exported but undocumented.


## [`createReadStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L167)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | fs.ReadStream |  |


## [`createWriteStream(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L207)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fscreatewritestreampath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | fs.WriteStream |  |


## [`fstat(fd, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L251)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fsfstatfd-options-callback

Invokes the callback with the <fs.Stats> for the file descriptor. See
 the POSIX fstat(2) documentation for more detail.



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false | A file descriptor. |
| options | Object |  | true | An options object. |
| callback | function |  | false | The function to call after completion. |


## [`open(path, flags , mode , callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L334)

Asynchronously open a file calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| flags | string | r | true |  |
| mode | string | 0o666 | true |  |
| callback | function(err, fd) |  | false |  |


## [`opendir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L387)

Asynchronously open a directory calling `callback` upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | Object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |
| callback | function(err, fd) |  | false |  |


## [`read(fd, buffer, offset, length, position, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L413)

Asynchronously read from an open file descriptor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| fd | number |  | false |  |
| buffer | object \| Buffer \| TypedArray |  | false | The buffer that the data will be written to. |
| offset | number |  | false | The position in buffer to write the data to. |
| length | number |  | false | The number of bytes to read. |
| position | number \| BigInt \| null |  | false | Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged. |
| callback | function(err, bytesRead, buffer) |  | false |  |


## [`readdir(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L447)

Asynchronously read all entries in a directory.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL  |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |
| callback | function(err, buffer) |  | false |  |


## [`readFile(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L498)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.flag | string | r | true |  |
| options.signal | AbortSignal |  | true |  |
| callback | function(err, buffer) |  | false |  |


## [`stat(path, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L571)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL \| number  |  | false | filename or file descriptor |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.flag | string | r | true |  |
| options.signal | AbortSignal |  | true |  |
| callback | function(err, data) |  | false |  |


## [`writeFile(path, data, options, callback)`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L641)



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


## [`writev()`](https://github.com/socketsupply/socket/blob/master/api/fs/index.js#L678)

This is a `FunctionDeclaration` named `writev` in `api/fs/index.js`, it's exported but undocumented.


# [FS.promises](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L4)



## [`access(path, mode, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L41)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesaccesspath-mode

Asynchronously check access a file.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | string |  | true |  |
| options | object |  | true |  |


## [`chmod(path, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L58)

External docs: https://nodejs.org/api/fs.html#fspromiseschmodpath-mode



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| mode | number |  | false |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<void> |  |


## [`mkdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L127)

Asynchronously creates a directory.


| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | String |  | false | The path to create |
| options | Object |  | false | The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false. |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Primise<any> | Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true. |


## [`open(path, flags, mode)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L151)

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


## [`opendir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L163)

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


## [`readdir(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L175)

External docs: https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesreaddirpath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |
| options.encoding | string | utf8 | true |  |
| options.withFileTypes | boolean | false | true |  |


## [`readFile(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L208)

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
| Not specified | Promise<Buffer | string> |  |


## [`stat(path, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L263)

External docs: https://nodejs.org/api/fs.html#fspromisesstatpath-options



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| path | string \| Buffer \| URL |  | false |  |
| options | object |  | true |  |
| options.bigint | boolean | false | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Stats> |  |


## [`writeFile(path, data, options)`](https://github.com/socketsupply/socket/blob/master/api/fs/promises.js#L316)

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


# [FS.Stream](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L4)



## [DEFAULT_STREAM_HIGH_WATER_MARK](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L8)

This is a `VariableDeclaration` named `DEFAULT_STREAM_HIGH_WATER_MARK` in `api/fs/stream.js`, it's exported but undocumented.


## [`ReadStream` (extends `Readable`)](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L13)

A `Readable` stream for a `FileHandle`.

### [`constructor()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L18)

`ReadStream` class constructor

### [`setHandle(handle)`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L54)

Sets file handle for the ReadStream.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| handle | FileHandle |  | false |  |


### [`highWaterMark()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L61)

The max buffer size for the ReadStream.

### [`path()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L68)

Relative or absolute path of the underlying `FileHandle`.

### [`pending()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L75)

`true` if the stream is in a pending state.

### [`emit()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L83)

Handles `shouldEmitClose` setting from `options.emitClose` in constructor.

## [`WriteStream` (extends `Writable`)](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L164)

A `Writable` stream for a `FileHandle`.

### [`constructor()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L169)

`WriteStream` class constructor

### [`setHandle(handle)`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L196)

Sets file handle for the WriteStream.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| handle | FileHandle |  | false |  |


### [`highWaterMark()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L203)

The max buffer size for the Writetream.

### [`path()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L210)

Relative or absolute path of the underlying `FileHandle`.

### [`pending()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L217)

`true` if the stream is in a pending state.

### [`emit()`](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L252)

Handles `shouldEmitClose` setting from `options.emitClose` in constructor.

## [FileReadStream](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L325)

This is a `VariableDeclaration` named `FileReadStream` in `api/fs/stream.js`, it's exported but undocumented.


## [FileWriteStream](https://github.com/socketsupply/socket/blob/master/api/fs/stream.js#L326)

This is a `VariableDeclaration` named `FileWriteStream` in `api/fs/stream.js`, it's exported but undocumented.


# [IPC](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L28)


 This is a low level API which you don't need unless you are implementing
 a library on top of Socket SDK. A Socket SDK app has two or three processes.

 - The `Render` process, the UI where the HTML, CSS and JS is run.
 - The `Bridge` process, the thin layer of code that managers everything.
 - The `Main` processs, for apps that need to run heavier compute jobs. And
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

## [`ready()`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L784)

Waits for the native IPC layer to be ready and exposed on the
 global window object.

## [`sendSync(command, params)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L809)

Sends a synchronous IPC command over XHR returning a `Result`
 upon success or error.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| command | string |  | false |  |
| params | object \| string |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Result |  |


## [`emit(name, value, target , options)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L854)

Emit event to be dispatched on `window` object.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| name | string |  | false |  |
| value | Mixed |  | false |  |
| target | EventTarget | window | true |  |
| options | Object |  | true |  |


## [`send(command, value)`](https://github.com/socketsupply/socket/blob/master/api/ipc.js#L910)

Sends an async IPC command request with parameters.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| command | string |  | false |  |
| value | Mixed |  | true |  |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Result> |  |


# [OS](https://github.com/socketsupply/socket/blob/master/api/os.js#L8)


 This module provides normalized system information from all the major
 operating systems.

## [`arch()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L20)

This is a `FunctionDeclaration` named `arch` in `api/os.js`, it's exported but undocumented.


## [`networkInterfaces()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L53)

This is a `FunctionDeclaration` named `networkInterfaces` in `api/os.js`, it's exported but undocumented.


## [`platform()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L141)

This is a `FunctionDeclaration` named `platform` in `api/os.js`, it's exported but undocumented.


## [`type()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L170)

This is a `FunctionDeclaration` named `type` in `api/os.js`, it's exported but undocumented.


## [`isWindows()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L210)

This is a `FunctionDeclaration` named `isWindows` in `api/os.js`, it's exported but undocumented.


## [`tmpdir()`](https://github.com/socketsupply/socket/blob/master/api/os.js#L219)

This is a `FunctionDeclaration` named `tmpdir` in `api/os.js`, it's exported but undocumented.


## [EOL](https://github.com/socketsupply/socket/blob/master/api/os.js#L263)

This is a `VariableDeclaration` named `EOL` in `api/os.js`, it's exported but undocumented.


# [Path](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L4)



## [Path](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L9)

This is a `ClassDeclaration` named `Path` in `api/path/path.js`, it's exported but undocumented.


### [`cwd(opts)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L15)

Computes current working directory for a path

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | true |  |
| opts.posix Set to `true` to force POSIX style path | boolean |  | true |  |


### [`constructor(opts)`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L44)

`Path` class constructor.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | true |  |
| opts.root | string |  | true |  |
| opts.base | string |  | true |  |
| opts.name | string |  | true |  |
| opts.dir | string |  | true |  |
| opts.ext | string |  | true |  |


### [`resolve()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L60)



### [`normalize()`](https://github.com/socketsupply/socket/blob/master/api/path/path.js#L66)



# [Process](https://github.com/socketsupply/socket/blob/master/api/process.js#L4)



## [`homedir()`](https://github.com/socketsupply/socket/blob/master/api/process.js#L31)



| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | string | The home directory of the current user. |


## [`exit(code)`](https://github.com/socketsupply/socket/blob/master/api/process.js#L38)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| code | number | 0 | true | The exit code. Default: 0. |


# [Runtime](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L7)


 Provides runtime-specific methods

## [currentWindow](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L10)

This is a `VariableDeclaration` named `currentWindow` in `api/runtime.js`, it's exported but undocumented.


## [debug](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L12)

This is a `VariableDeclaration` named `debug` in `api/runtime.js`, it's exported but undocumented.


## [config](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L14)

This is a `VariableDeclaration` named `config` in `api/runtime.js`, it's exported but undocumented.


## [`send()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L24)

This is a `FunctionDeclaration` named `send` in `api/runtime.js`, it's exported but undocumented.


## [`getWindows()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L40)

This is a `FunctionDeclaration` named `getWindows` in `api/runtime.js`, it's exported but undocumented.


## [`openExternal()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L44)

This is a `FunctionDeclaration` named `openExternal` in `api/runtime.js`, it's exported but undocumented.


## [`exit(options)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L53)

Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Any> |  |


## [`setTitle(options)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L62)

Sets the title of the window (if applicable).

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | obnject |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`inspect()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L66)

This is a `FunctionDeclaration` named `inspect` in `api/runtime.js`, it's exported but undocumented.


## [`show(opts)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L76)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`hide(opts)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L89)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`navigate(opts)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L100)



| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| opts | object |  | false | an options object |
| opts.window | number | currentWindow | false | the index of the window |
| opts.url | number |  | false | the path to the HTML file to load into the window |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<ipc.Result> |  |


## [`setWindowBackgroundColor()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L109)

This is a `FunctionDeclaration` named `setWindowBackgroundColor` in `api/runtime.js`, it's exported but undocumented.


## [`setContextMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L120)

Opens a native context menu.

| Argument | Type | Default | Optional | Description |
| :---     | :--- | :---:   | :---:    | :---        |
| options | object |  | false | an options object |


| Return Value | Type | Description |
| :---         | :--- | :---        |
| Not specified | Promise<Any> |  |


## [`setSystemMenuItemEnabled()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L128)

This is a `FunctionDeclaration` named `setSystemMenuItemEnabled` in `api/runtime.js`, it's exported but undocumented.


## [`setSystemMenu(options)`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L217)

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
 socket.runtime.setSystemMenu({ index: 0, value: `
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

 #### Separators

 To create a separator, use three dashes `---`.

 #### Accelerator Modifiers

 Accelerator modifiers are used as visual indicators but don't have a
 material impact as the actual key binding is done in the event listener.

 A capital letter implies that the accelerator is modified by the `Shift` key.

 Additional accelerators are `Meta`, `Control`, `Option`, each separated
 by commas. If one is not applicable for a platform, it will just be ignored.

 On MacOS `Meta` is the same as `Command`.

 #### Disabled Items

 If you want to disable a menu item just prefix the item with the `!` character.
 This will cause the item to appear disabled when the system menu renders.

 #### Submenus

 We feel like nested menus are an anti-pattern. We don't use them. If you have a
 strong argument for them and a very simple pull request that makes them work we
 may consider them.

 #### Event Handling

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
| Not specified | Promise<Any> |  |


## [`reload()`](https://github.com/socketsupply/socket/blob/master/api/runtime.js#L266)

This is a `FunctionDeclaration` named `reload` in `api/runtime.js`, it's exported but undocumented.


# [Stream](https://github.com/socketsupply/socket/blob/master/api/stream.js#L5)


