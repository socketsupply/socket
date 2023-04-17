/**
 * @module Dgram
 *
 * This module provides an implementation of UDP datagram sockets. It does
 * not (yet) provide any of the multicast methods or properties.
 *
 * Example usage:
 * ```js
 * import { createSocket } from 'socket:dgram'
 * ```
 */

import { isArrayBufferView, isFunction, noop } from './util.js'
import { InternalError } from './errors.js'
import { EventEmitter } from './events.js'
import diagnostics from './diagnostics.js'
import { Buffer } from './buffer.js'
import { rand64 } from './crypto.js'
import { isIPv4 } from './net.js'
import process from './process.js'
import console from './console.js'
import ipc from './ipc.js'
import dns from './dns.js'
import gc from './gc.js'

import * as exports from './dgram.js'

const BIND_STATE_UNBOUND = 0
const BIND_STATE_BINDING = 1
const BIND_STATE_BOUND = 2

const CONNECT_STATE_DISCONNECTED = 0
const CONNECT_STATE_CONNECTING = 1
const CONNECT_STATE_CONNECTED = 2

const MAX_PORT = 64 * 1024
const RECV_BUFFER = 1
const SEND_BUFFER = 0

const dc = diagnostics.channels.group('udp', [
  'send',
  'send.start',
  'send.end',
  'message',
  'connect',
  'socket',
  'close',
  'bind'
])

function defaultCallback (socket) {
  return (err) => {
    if (err) socket.emit('error', err)
  }
}

function createDataListener (socket) {
  // subscribe this socket to the firehose
  globalThis.addEventListener('data', ondata)
  return ondata

  function ondata ({ detail }) {
    const { err, data, source } = detail.params
    const buffer = detail.data

    if (err && err.id === socket.id) {
      return socket.emit('error', err)
    }

    if (!data || BigInt(data.id) !== socket.id) return

    if (source === 'udp.readStart') {
      const message = Buffer.from(buffer)
      const info = {
        ...data,
        family: getAddressFamily(data.address)
      }

      socket.emit('message', message, info)
      dc.channel('message').publish({ socket, buffer: message, info })
    }

    if (data.EOF) {
      globalThis.removeEventListener('data', ondata)
    }
  }
}

function destroyDataListener (socket) {
  if (typeof socket?.dataListener === 'function') {
    globalThis.removeEventListener('data', socket.dataListener)
    delete socket.dataListener
  }
}

function fromBufferList (list) {
  const newlist = new Array(list.length)

  for (let i = 0, l = list.length; i < l; i++) {
    const buf = list[i]

    if (typeof buf === 'string') {
      newlist[i] = Buffer.from(buf)
    } else if (!isArrayBufferView(buf)) {
      return null
    } else {
      newlist[i] = Buffer.from(buf.buffer, buf.byteOffset, buf.byteLength)
    }
  }

  return newlist
}

function getDefaultAddress (socket) {
  if (socket.type === 'udp6') return '::1'
  if (socket.type === 'udp4') return '0.0.0.0'

  return null
}

function getAddressFamily (address) {
  return isIPv4(address) ? 'IPv4' : 'IPv6'
}

function getSocketState (socket) {
  const result = ipc.sendSync('udp.getState', { id: socket.id })

  if (result.err && result.err.code !== 'NOT_FOUND_ERR') {
    throw result.err
  }

  return result.data || null
}

// eslint-disable-next-line no-unused-vars
function healhCheck (socket) {
  // @TODO(jwerle)
}

async function startReading (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = await ipc.send('udp.readStart', {
      id: socket.id
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
  }

  return result
}

async function stopReading (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = await ipc.send('udp.readStop', {
      id: socket.id
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
  }

  return result
}

async function getRecvBufferSize (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = await ipc.send('os.bufferSize', {
      id: socket.id,
      buffer: RECV_BUFFER
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  return result
}

async function getSendBufferSize (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = await ipc.send('os.bufferSize', {
      id: socket.id,
      buffer: SEND_BUFFER
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  return result
}

async function bind (socket, options, callback) {
  let result = null

  options = { ...options }

  if (!isFunction(callback)) {
    callback = noop
  }

  if (typeof options.address !== 'string') {
    options.address = getDefaultAddress(socket)
  }

  socket.state.bindState = BIND_STATE_BINDING

  if (typeof options.address === 'string' && !isIPv4(options.address)) {
    try {
      options.address = await dns.lookup(options.address, 4)
    } catch (err) {
      socket.state.bindState = BIND_STATE_UNBOUND
      callback(err)
      return { err }
    }
  }

  try {
    result = await ipc.send('udp.bind', {
      id: socket.id,
      port: options.port || 0,
      address: options.address,
      ipv6Only: !!options.ipv6Only,
      reuseAddr: !!options.reuseAddr
    })

    socket.state.bindState = BIND_STATE_BOUND

    if (socket.state.sendBufferSize) {
      await socket.setSendBufferSize(socket.state.sendBufferSize)
    } else {
      const result = await getSendBufferSize(socket)
      if (result.err) {
        callback(result.err)
        return { err: result.err }
      }

      socket.state.sendBufferSize = result.data.size
    }

    if (socket.state.recvBufferSize) {
      await socket.setRecvBufferSize(socket.state.recvBufferSize)
    } else {
      const result = await getRecvBufferSize(socket)
      if (result.err) {
        callback(result.err)
        return { err: result.err }
      }

      socket.state.recvBufferSize = result.data.size
    }

    callback(result.err, result.data)
  } catch (err) {
    socket.state.bindState = BIND_STATE_UNBOUND
    callback(err)
    return { err }
  }

  dc.channel('bind').publish({
    socket,
    port: options.port || 0,
    address: options.address,
    ipv6Only: !!options.ipv6Only,
    reuseAddr: !!options.reuseAddr
  })

  return result
}

async function connect (socket, options, callback) {
  let result = null

  options = { ...options }

  if (!isFunction(callback)) {
    callback = noop
  }

  if (typeof options.address !== 'string') {
    options.address = getDefaultAddress(socket)
  }

  socket.state.connectState = CONNECT_STATE_CONNECTING

  if (typeof options.address === 'string' && !isIPv4(options.address)) {
    try {
      options.address = await dns.lookup(options.address, 4)
    } catch (err) {
      socket.state.connectState = CONNECT_STATE_DISCONNECTED
      callback(err)
      return { err }
    }
  }

  const { err } = await bind(socket, { port: 0 })

  if (err) {
    socket.state.connectState = CONNECT_STATE_DISCONNECTED
    callback(err)
    return { err }
  }

  try {
    result = await ipc.send('udp.connect', {
      id: socket.id,
      port: options?.port ?? 0,
      address: options?.address
    })

    socket.state.connectState = CONNECT_STATE_CONNECTED

    if (socket.state.sendBufferSize) {
      await socket.setSendBufferSize(socket.state.sendBufferSize)
    } else {
      const result = await getSendBufferSize(socket)
      if (result.err) {
        callback(result.err)
        return { err }
      }

      socket.state.sendBufferSize = result.data.size
    }

    if (socket.state.recvBufferSize) {
      await socket.setRecvBufferSize(socket.state.recvBufferSize)
    } else {
      const result = await getRecvBufferSize(socket)
      if (result.err) {
        callback(result.err)
        return { err }
      }

      socket.state.recvBufferSize = result.data.size
    }

    callback(result.err, result.data)
  } catch (err) {
    socket.state.connectState = CONNECT_STATE_DISCONNECTED
    callback(err)
    return { err }
  }

  dc.channel('connect').publish({
    socket,
    port: options?.port ?? 0,
    address: options?.address
  })

  return result
}

function disconnect (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  if (socket.state.connectState === CONNECT_STATE_CONNECTED) {
    try {
      result = ipc.sendSync('udp.disconnect', {
        id: socket.id
      })

      delete socket.state.remoteAddress
      socket.state.connectState = CONNECT_STATE_DISCONNECTED

      callback(result.err, result.data)
    } catch (err) {
      callback(err)
      return { err }
    }

    dc.channel('disconnect').publish({ socket })
  }

  return result
}

async function send (socket, options, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  options = { ...options }

  if (socket.state.connectState === CONNECT_STATE_DISCONNECTED) {
    // wait for bind to finish
    if (socket.state.bindState === BIND_STATE_BINDING) {
      const { err } = await new Promise((resolve, reject) => {
        socket.once('listening', () => resolve({}))
        socket.once('error', (err) => resolve({ err }))
      })

      if (err) {
        callback(err)
        return { err }
      }
    } else if (socket.state.bindState === BIND_STATE_UNBOUND) {
      const { err } = await bind(socket, { port: 0 })
      if (err) {
        callback(err)
        return { err }
      }
    }
  }

  // wait for connect to finish
  if (socket.state.connectState === CONNECT_STATE_CONNECTING) {
    const { err } = await new Promise((resolve, reject) => {
      socket.once('connect', () => resolve({}))
      socket.once('error', (err) => resolve({ err }))
    })

    if (err) {
      callback(err)
      return { err }
    }
  }

  if (
    !isIPv4(options.address) &&
    typeof options.address === 'string' &&
    socket.state.connectState !== CONNECT_STATE_CONNECTED
  ) {
    try {
      options.address = await dns.lookup(options.address, 4)
    } catch (err) {
      callback(err)
      return { err }
    }
  }

  // use local port/address if not given
  if (socket.state.bindState === BIND_STATE_BOUND) {
    const local = socket.address()
    if (!options.address) {
      options.address = local.address
    }

    if (!options.port) {
      options.port = local.port
    }
  }

  // use remote port/address if not given
  if (socket.state.connectState === CONNECT_STATE_CONNECTED) {
    const remote = socket.remoteAddress()
    if (!options.address) {
      options.address = remote.address
    }

    if (!options.port) {
      options.port = remote.port
    }
  }

  if (options.port && !options.address) {
    options.address = getDefaultAddress(socket)
  }

  try {
    dc.channel('send.start').publish({
      socket,
      port: options.port,
      buffer: options.buffer,
      address: options.address
    })

    result = await ipc.write('udp.send', {
      id: socket.id,
      port: options.port,
      address: options.address
    }, options.buffer)

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  dc.channel('send.end').publish({
    socket,
    port: options.port,
    buffer: options.buffer,
    address: options.address
  })

  dc.channel('send').publish({
    socket,
    port: options.port,
    buffer: options.buffer,
    address: options.address
  })

  return result
}

async function close (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  socket.state.connectState = CONNECT_STATE_DISCONNECTED
  socket.state.bindState = BIND_STATE_UNBOUND

  await stopReading(socket)
  await disconnect(socket)

  try {
    result = await ipc.send('udp.close', {
      id: socket.id
    })

    gc.unref(socket)

    delete socket.state.address
    delete socket.state.remoteAddress
    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  dc.channel('close').publish({ socket })
  return result
}

function getPeerName (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = ipc.sendSync('udp.getPeerName', {
      id: socket.id
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  return result
}

function getSockName (socket, callback) {
  let result = null

  if (!isFunction(callback)) {
    callback = noop
  }

  try {
    result = ipc.sendSync('udp.getSockName', {
      id: socket.id
    })

    callback(result.err, result.data)
  } catch (err) {
    callback(err)
    return { err }
  }

  return result
}

/**
 * @typedef {Object} SocketOptions
 */

/**
 * Creates a `Socket` instance.
 * @param {string|Object} options - either a string ('udp4' or 'udp6') or an options object
 * @param {string=} options.type - The family of socket. Must be either 'udp4' or 'udp6'. Required.
 * @param {boolean=} [options.reuseAddr=false] - When true socket.bind() will reuse the address, even if another process has already bound a socket on it. Default: false.
 * @param {boolean=} [options.ipv6Only=false] - Default: false.
 * @param {number=} options.recvBufferSize - Sets the SO_RCVBUF socket value.
 * @param {number=} options.sendBufferSize - Sets the SO_SNDBUF socket value.
 * @param {AbortSignal=} options.signal - An AbortSignal that may be used to close a socket.
 * @param {function=} callback - Attached as a listener for 'message' events. Optional.
 * @return {Socket}
 */
export const createSocket = (options, callback) => new Socket(options, callback)

/**
 * New instances of dgram.Socket are created using dgram.createSocket().
 * The new keyword is not to be used to create dgram.Socket instances.
 */
export class Socket extends EventEmitter {
  constructor (options, callback) {
    super()

    this.id = rand64()

    if (typeof options === 'string') {
      options = { type: options }
    }

    options = { ...options }

    if (!['udp4', 'udp6'].includes(options.type)) {
      throw new ERR_SOCKET_BAD_TYPE()
    }

    this.type = options.type
    this.signal = options?.signal ?? null

    this.state = {
      recvBufferSize: options.recvBufferSize,
      sendBufferSize: options.sendBufferSize,
      bindState: BIND_STATE_UNBOUND,
      connectState: CONNECT_STATE_DISCONNECTED,
      reuseAddr: options.reuseAddr === true,
      ipv6Only: options.ipv6Only === true
    }

    if (isFunction(callback)) {
      this.on('message', callback)
    }

    const onabort = () => this.close()
    this.signal?.addEventListener('abort', onabort, { once: true })
    this.once('close', () => {
      destroyDataListener(this)
      this.removeAllListeners()
      this.signal?.removeEventListener('abort', onabort)
    })

    gc.ref(this, options)
    dc.channel('socket').publish({ socket: this })
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, options],
      async handle (id) {
        if (process.env.DEBUG) {
          console.warn('Closing Socket on garbage collection')
        }

        await ipc.send('udp.close', { id }, options)
      }
    }
  }

  /**
   * Listen for datagram messages on a named port and optional address
   * If the address is not specified, the operating system will attempt to
   * listen on all addresses. Once the binding is complete, a 'listening'
   * event is emitted and the optional callback function is called.
   *
   * If binding fails, an 'error' event is emitted.
   *
   * @param {number} port - The port to listen for messages on
   * @param {string} address - The address to bind to (0.0.0.0)
   * @param {function} callback - With no parameters. Called when binding is complete.
   * @see {@link https://nodejs.org/api/dgram.html#socketbindport-address-callback}
   */
  bind (arg1, arg2, arg3) {
    const options = {}
    const cb = isFunction(arg2)
      ? arg2
      : isFunction(arg3)
        ? arg3
        : defaultCallback(this)

    if (typeof arg1 === 'number' || typeof arg2 === 'string') {
      options.port = parseInt(arg1)
    } else if (typeof arg1 === 'object') {
      Object.assign(options, arg1)
    }

    if (typeof arg2 === 'string') {
      options.address = arg2
    }

    bind(this, options, (err, info) => {
      if (err) {
        return cb(err)
      }

      startReading(this, (err) => {
        if (err) {
          cb(err)
        } else {
          this.dataListener = createDataListener(this)
          cb(null)
          this.emit('listening')
        }
      })
    })

    return this
  }

  /**
   * Associates the dgram.Socket to a remote address and port. Every message sent
   * by this handle is automatically sent to that destination. Also, the socket
   * will only receive messages from that remote peer. Trying to call connect()
   * on an already connected socket will result in an ERR_SOCKET_DGRAM_IS_CONNECTED
   * exception. If the address is not provided, '0.0.0.0' (for udp4 sockets) or '::1'
   * (for udp6 sockets) will be used by default. Once the connection is complete,
   * a 'connect' event is emitted and the optional callback function is called.
   * In case of failure, the callback is called or, failing this, an 'error' event
   * is emitted.
   *
   * @param {number} port - Port the client should connect to.
   * @param {string=} host - Host the client should connect to.
   * @param {function=} connectListener - Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once.
   * @see {@link https://nodejs.org/api/dgram.html#socketconnectport-address-callback}
   */
  connect (arg1, arg2, arg3) {
    const address = isFunction(arg2) ? undefined : arg2
    const port = parseInt(arg1)
    const cb = isFunction(arg2)
      ? arg2
      : isFunction(arg3)
        ? arg3
        : defaultCallback(this)

    if (!Number.isInteger(port) || port <= 0 || port > MAX_PORT) {
      throw new ERR_SOCKET_BAD_PORT(
        `Port should be > 0 and < 65536. Received ${arg1}.`
      )
    }

    if (this.state.connectState !== CONNECT_STATE_DISCONNECTED) {
      throw new ERR_SOCKET_DGRAM_IS_CONNECTED()
    }

    connect(this, { address, port }, (err, info) => {
      cb(err, info)

      if (!err && info) {
        this.emit('connect', info)
      }
    })
  }

  /**
   * A synchronous function that disassociates a connected dgram.Socket from
   * its remote address. Trying to call disconnect() on an unbound or already
   * disconnected socket will result in an ERR_SOCKET_DGRAM_NOT_CONNECTED exception.
   *
   * @see {@link https://nodejs.org/api/dgram.html#socketdisconnect}
   */
  disconnect () {
    if (this.state.connectState !== CONNECT_STATE_CONNECTED) {
      throw new ERR_SOCKET_DGRAM_NOT_CONNECTED()
    }

    const { err } = disconnect(this)

    if (err) {
      throw err
    }
  }

  /**
   * Broadcasts a datagram on the socket. For connectionless sockets, the
   * destination port and address must be specified. Connected sockets, on the
   * other hand, will use their associated remote endpoint, so the port and
   * address arguments must not be set.
   *
   * > The msg argument contains the message to be sent. Depending on its type,
   * different behavior can apply. If msg is a Buffer, any TypedArray, or a
   * DataView, the offset and length specify the offset within the Buffer where
   * the message begins and the number of bytes in the message, respectively.
   * If msg is a String, then it is automatically converted to a Buffer with
   * 'utf8' encoding. With messages that contain multi-byte characters, offset,
   * and length will be calculated with respect to byte length and not the
   * character position. If msg is an array, offset and length must not be
   * specified.
   *
   * > The address argument is a string. If the value of the address is a hostname,
   * DNS will be used to resolve the address of the host. If the address is not
   * provided or otherwise nullish, '0.0.0.0' (for udp4 sockets) or '::1'
   * (for udp6 sockets) will be used by default.
   *
   * > If the socket has not been previously bound with a call to bind, the socket
   * is assigned a random port number and is bound to the "all interfaces"
   * address ('0.0.0.0' for udp4 sockets, '::1' for udp6 sockets.)
   *
   * > An optional callback function may be specified as a way of reporting DNS
   * errors or for determining when it is safe to reuse the buf object. DNS
   * lookups delay the time to send for at least one tick of the Node.js event
   * loop.
   *
   * > The only way to know for sure that the datagram has been sent is by using a
   * callback. If an error occurs and a callback is given, the error will be
   * passed as the first argument to the callback. If a callback is not given,
   * the error is emitted as an 'error' event on the socket object.
   *
   * > Offset and length are optional but both must be set if either is used.
   * They are supported only when the first argument is a Buffer, a TypedArray,
   * or a DataView.
   *
   * @param {Buffer | TypedArray | DataView | string | Array} msg - Message to be sent.
   * @param {integer=} offset - Offset in the buffer where the message starts.
   * @param {integer=} length - Number of bytes in the message.
   * @param {integer=} port - Destination port.
   * @param {string=} address - Destination host name or IP address.
   * @param {Function=} callback - Called when the message has been sent.
   * @see {@link https://nodejs.org/api/dgram.html#socketsendmsg-offset-length-port-address-callback}
   */
  send (buffer, ...args) {
    const id = this.id || rand64()
    let offset = 0
    let length
    let port
    let address
    let cb = defaultCallback(this)

    if (Array.isArray(buffer)) {
      buffer = fromBufferList(buffer)
    } else if (typeof buffer === 'string' || isArrayBufferView(buffer)) {
      buffer = Buffer.from(buffer)
    }

    if (!Buffer.isBuffer(buffer)) {
      throw new TypeError('Invalid buffer')
    }

    // detect callback at end of `args`
    if (args.findIndex(isFunction) === args.length - 1) {
      cb = args.pop()
    }

    // parse argument variants
    if (typeof args[2] === 'number') {
      [offset, length, port, address] = args
    } else if (typeof args[1] === 'number') {
      [offset, length] = args
    } else if (typeof args[0] === 'number' && typeof args[1] === 'string') {
      [port, address] = args
    } else if (typeof args[0] === 'number') {
      [port] = args
    }

    if (port !== undefined || this.state.connectState !== CONNECT_STATE_CONNECTED) {
      // parse possible port as string
      port = parseInt(port)

      if (!Number.isInteger(port) || port <= 0 || port > (64 * 1024)) {
        throw new ERR_SOCKET_BAD_PORT(
          `Port should be > 0 and < 65536. Received ${port}.`
        )
      }
    }

    if (offset === undefined) {
      offset = 0
    }

    if (length === undefined) {
      length = buffer.length
    }

    if (!Number.isInteger(offset) || offset < 0) {
      throw new RangeError(
        `Offset should be >= 0 and < ${buffer.length} Received ${offset}.`
      )
    }

    buffer = buffer.slice(offset)

    if (!Number.isInteger(length) || length < 0 || length > buffer.length) {
      throw new RangeError(
        `Length should be >= 0 and <= ${buffer.length} Received ${length}.`
      )
    }

    buffer = buffer.slice(0, length)

    return send(this, { id, port, address, buffer }, cb)
  }

  /**
   * Close the underlying socket and stop listening for data on it. If a
   * callback is provided, it is added as a listener for the 'close' event.
   *
   * @param {function=} callback - Called when the connection is completed or on error.
   *
   * @see {@link https://nodejs.org/api/dgram.html#socketclosecallback}
   */
  close (cb) {
    const state = getSocketState(this)

    if (
      !state || !(state.connected || state.bound) ||
      (
        this.state.bindState === BIND_STATE_UNBOUND &&
        this.state.connectState === CONNECT_STATE_DISCONNECTED
      )
    ) {
      if (isFunction(cb)) {
        cb(new ERR_SOCKET_DGRAM_NOT_RUNNING())
        return
      } else {
        throw new ERR_SOCKET_DGRAM_NOT_RUNNING()
      }
    }

    close(this, (err) => {
      if (err) {
        // gc might have already closed this
        if (!gc.finalizers.has(this)) {
          if (isFunction(cb)) {
            cb()
            return
          }
        }

        if (err.code === 'ERR_SOCKET_DGRAM_NOT_RUNNING') {
          err = Object.assign(new ERR_SOCKET_DGRAM_NOT_RUNNING(), {
            cause: err
          })
        }

        if (isFunction(cb)) {
          cb(err)
        } else {
          this.emit('error', err)
        }

        return
      }

      if (isFunction(cb)) {
        cb(null)
      }

      this.emit('close')
    })

    return this
  }

  /**
   *
   * Returns an object containing the address information for a socket. For
   * UDP sockets, this object will contain address, family, and port properties.
   *
   * This method throws EBADF if called on an unbound socket.
   * @returns {Object} socketInfo - Information about the local socket
   * @returns {string} socketInfo.address - The IP address of the socket
   * @returns {string} socketInfo.port - The port of the socket
   * @returns {string} socketInfo.family - The IP family of the socket
   *
   * @see {@link https://nodejs.org/api/dgram.html#socketaddress}
   */
  address () {
    if (this.state.bindState !== BIND_STATE_BOUND) {
      throw new ERR_SOCKET_DGRAM_NOT_RUNNING()
    }

    if (!this.state.address) {
      const result = getSockName(this)

      if (result.err) {
        throw Object.assign(result.err, {
          syscall: 'getsockname'
        })
      }

      this.state.address = result.data
    }

    return {
      port: this.state.address?.port ?? null,
      family: this.state.address?.family ?? null,
      address: this.state.address?.address ?? null
    }
  }

  /**
   * Returns an object containing the address, family, and port of the remote
   * endpoint. This method throws an ERR_SOCKET_DGRAM_NOT_CONNECTED exception
   * if the socket is not connected.
   *
   * @returns {Object} socketInfo - Information about the remote socket
   * @returns {string} socketInfo.address - The IP address of the socket
   * @returns {string} socketInfo.port - The port of the socket
   * @returns {string} socketInfo.family - The IP family of the socket
   * @see {@link https://nodejs.org/api/dgram.html#socketremoteaddress}
   */
  remoteAddress () {
    if (this.state.connectState !== CONNECT_STATE_CONNECTED) {
      throw new ERR_SOCKET_DGRAM_NOT_CONNECTED()
    }

    if (!this.state.remoteAddress) {
      const result = getPeerName(this)

      if (result.err) {
        throw Object.assign(result.err, {
          syscall: 'getpeername'
        })
      }

      this.state.remoteAddress = result.data
    }

    return {
      port: this.state.remoteAddress?.port ?? null,
      family: this.state.remoteAddress?.family ?? null,
      address: this.state.remoteAddress?.address ?? null
    }
  }

  /**
   * Sets the SO_RCVBUF socket option. Sets the maximum socket receive buffer in
   * bytes.
   *
   * @param {number} size - The size of the new receive buffer
   * @see {@link https://nodejs.org/api/dgram.html#socketsetrecvbuffersizesize}
   */
  async setRecvBufferSize (size) {
    if (size > 0) {
      this.state.recvBufferSize = size
      const result = await ipc.send('os.bufferSize', { id: this.id, size })
      if (result.err) {
        throw result.err
      }
    }
  }

  /**
   * Sets the SO_SNDBUF socket option. Sets the maximum socket send buffer in
   * bytes.
   *
   * @param {number} size - The size of the new send buffer
   * @see {@link https://nodejs.org/api/dgram.html#socketsetsendbuffersizesize}
   */
  async setSendBufferSize (size) {
    if (size > 0) {
      this.state.sendBufferSize = size
      const result = await ipc.send('os.bufferSize', { id: this.id, size })
      if (result.err) {
        throw result.err
      }
    }
  }

  /**
   * @see {@link https://nodejs.org/api/dgram.html#socketgetrecvbuffersize}
   */
  getRecvBufferSize () {
    return this.state.recvBufferSize
  }

  /**
   * @returns {number} the SO_SNDBUF socket send buffer size in bytes.
   * @see {@link https://nodejs.org/api/dgram.html#socketgetsendbuffersize}
   */
  getSendBufferSize () {
    return this.state.sendBufferSize
  }

  //
  // For now we aren't going to implement any of the multicast options,
  // mainly because 1. we don't need it in hyper and 2. if a user wants
  // to deploy their app to the app store, they will need to request the
  // multicast entitlement from apple. If someone really wants this they
  // can implement it.
  //
  setBroadcast () {
    throw new Error('not implemented')
  }

  setTTL () {
    throw new Error('not implemented')
  }

  setMulticastTTL () {
    throw new Error('not implemented')
  }

  setMulticastLoopback () {
    throw new Error('not implemented')
  }

  setMulticastMembership () {
    throw new Error('not implemented')
  }

  setMulticastInterface () {
    throw new Error('not implemented')
  }

  addMembership () {
    throw new Error('not implemented')
  }

  dropMembership () {
    throw new Error('not implemented')
  }

  addSourceSpecificMembership () {
    throw new Error('not implemented')
  }

  dropSourceSpecificMembership () {
    throw new Error('not implemented')
  }

  ref () {
    return this
  }

  unref () {
    return this
  }
}

/**
 * Generic error class for an error occurring on a `Socket` instance.
 * @ignore
 */
export class SocketError extends InternalError {
  get code () { return this.constructor.name }
}

/**
 * Thrown when a socket is already bound.
 */
export class ERR_SOCKET_ALREADY_BOUND extends SocketError {
  get message () { return 'Socket is already bound' }
}

/**
 * @ignore
 */
export class ERR_SOCKET_BAD_BUFFER_SIZE extends SocketError {}

/**
 * @ignore
 */
export class ERR_SOCKET_BUFFER_SIZE extends SocketError {}

/**
 * Thrown when the socket is already connected.
 */
export class ERR_SOCKET_DGRAM_IS_CONNECTED extends SocketError {
  get message () { return 'Alread connected' }
}

/**
 * Thrown when the socket is not connected.
 */
export class ERR_SOCKET_DGRAM_NOT_CONNECTED extends SocketError {
  syscall = 'getpeername'
  get message () { return 'Not connected' }
}

/**
 * Thrown when the socket is not running (not bound or connected).
 */
export class ERR_SOCKET_DGRAM_NOT_RUNNING extends SocketError {
  get message () { return 'Not running' }
}

/**
 * Thrown when a bad socket type is used in an argument.
 */
export class ERR_SOCKET_BAD_TYPE extends TypeError {
  code = 'ERR_SOCKET_BAD_TYPE'
  get message () {
    return 'Bad socket type specified. Valid types are: udp4, udp6'
  }
}

/**
 * Thrown when a bad port is given.
 */
export class ERR_SOCKET_BAD_PORT extends RangeError {
  code = 'ERR_SOCKET_BAD_PORT'
}

export default exports
