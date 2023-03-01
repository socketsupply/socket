/**
 * @module Net
 *
 * THIS MODULE HAS BEEN DISABLED!
 *
 * This module provides an asynchronous network API for creating
 * stream-based TCP or IPC servers (net.createServer()) and clients
 * (net.createConnection()).
 *
 */

import { EventEmitter } from './events.js'
import { Duplex } from './stream.js'
import { rand64 } from './crypto.js'
import console from './console.js'
import * as exports from './net.js'

export default exports

const assertType = (name, expected, actual, code) => {
  const msg = `'${name}' must be a '${expected}', received '${actual}'`
  const err = new TypeError(msg)
  err.code = code
  throw err
}

// lifted from nodejs/node/
const normalizedArgsSymbol = Symbol('normalizedArgsSymbol')
const kLastWriteQueueSize = Symbol('lastWriteQueueSize')

const normalizeArgs = (args) => {
  let arr

  if (args.length === 0) {
    arr = [{}, null]
    arr[normalizedArgsSymbol] = true
    return arr
  }

  const arg0 = args[0]
  let options = {}

  if (typeof arg0 === 'object' && arg0 !== null) {
    // (options[...][, cb])
    options = arg0

  // not supported: pipes
  //  } else if (isPipeName(arg0)) {
  //    // (path[...][, cb])
  //    options.path = arg0
  } else {
    // ([port][, host][...][, cb])
    options.port = arg0

    if (args.length > 1 && typeof args[1] === 'string') {
      options.host = args[1]
    }
  }

  const cb = args[args.length - 1]
  if (typeof cb !== 'function') {
    arr = [options, null]
  } else {
    arr = [options, cb]
  }

  arr[normalizedArgsSymbol] = true
  return arr
}

export class Server extends EventEmitter {
  constructor (options, handler) {
    super()

    if (typeof options === 'undefined') {
      options = handler
    }

    this._connections = 0
    this.id = rand64()
  }

  onconnection (data) {
    const socket = new Socket(data)

    if (this.maxConnections && this._connections >= this.maxConnections) {
      socket.close(data)
      return
    }

    this._connections++
    socket._server = this

    this.emit('connection', socket)
  }

  listen (port, address, cb) {
    ;(async opts => {
      const { err, data } = await window._ipc.send('tcpCreateServer', opts)

      if (err && !cb) {
        this.emit('error', err)
        return
      }

      this._address = { port: data.port, address: data.address, family: data.family }
      this.connections = {}

      window._ipc.streams[opts.id] = this

      if (cb) return cb(null, data)
      this.emit('listening', data)
    })({ port, address, id: this.id })

    return this
  }

  address () {
    return this._address
  }

  close (cb) {
    const params = {
      id: this.id
    }

    ;(async () => {
      const { err } = await window._ipc.send('tcpClose', params)
      delete window._ipc.streams[this.id]
      if (err && !cb) this.emit('error', err)
      else if (cb) cb(err)
    })()
  }

  getConnections (cb) {
    assertType('Callback', 'function', typeof cb, 'ERR_INVALID_CALLBACK')
    const params = {
      id: this.id
    }

    ;(async () => {
      const {
        err,
        data
      } = await window._ipc.send('tcpServerGetConnections', params)

      if (cb) cb(err, data)
    })()
  }

  unref () {
    return this
  }
}

export class Socket extends Duplex {
  constructor (options) {
    super()

    this._server = null

    this._address = null
    this.allowHalfOpen = options.allowHalfOpen === true
    this._flowing = false
    /*
    this.on('end', () => {
      if (!this.allowHalfOpen)
        this.writable = false
        //this.write = this._writeAfterFIN;
    })
    */
  }

  // note: this is not an async method on node, so it's not here
  // thus the ipc response is not awaited. since _ipc.send is async
  // but the messages are handled in order, you do not need to wait
  // for it before sending data, noDelay will be set correctly before the
  // next data is sent.
  setNoDelay (enable) {
    const params = {
      id: this.id, enable
    }
    window._ipc.send('tcpSetNoDelay', params)
  }

  // note: see note for setNoDelay
  setKeepAlive (enabled) {
    const params = {
      id: this.id,
      enabled
    }

    window._ipc.send('tcpSetKeepAlive', params)
  }

  _onTimeout () {
    const handle = this._handle
    const lastWriteQueueSize = this[kLastWriteQueueSize]

    if (lastWriteQueueSize > 0 && handle) {
      // `lastWriteQueueSize !== writeQueueSize` means there is
      // an active write in progress, so we suppress the timeout.
      const { writeQueueSize } = handle

      if (lastWriteQueueSize !== writeQueueSize) {
        this[kLastWriteQueueSize] = writeQueueSize
        this._unrefTimer()
        return
      }
    }

    this.emit('timeout')
  }

  address () {
    return { ...this._address }
  }

  _final (cb) {
    if (this.pending) {
      return this.once('connect', () => this._final(cb))
    }

    const params = {
      id: this.id
    }
    ;(async () => {
      const { err, data } = await window._ipc.send('tcpShutdown', params)
      if (cb) cb(err, data)
    })()
  }

  _destroy (cb) {
    if (this.destroyed) return
    ;(async () => {
      await window._ipc.send('tcpClose', { id: this.id })
      if (this._server) {
        this._server._connections--

        if (this._server._connections === 0) {
          this._server.emit('close')
        }
      }
      cb()
    })()
  }

  destroySoon () {
    if (this.writable) this.end()

    if (this.writableFinished) {
      this.destroy()
    } else {
      this.once('finish', this.destroy)
    }
  }

  _writev (data, cb) {
    ;(async () => {
      const allBuffers = data.allBuffers
      let chunks

      if (allBuffers) {
        chunks = data
        for (let i = 0; i < data.length; i++) {
          data[i] = data[i].chunk
        }
      } else {
        chunks = new Array(data.length << 1)

        for (let i = 0; i < data.length; i++) {
          const entry = data[i]
          chunks[i * 2] = entry.chunk
          chunks[i * 2 + 1] = entry.encoding
        }
      }

      const requests = []

      for (const chunk of chunks) {
        const params = {
          id: this.id,
          data: chunk
        }
        // sent in order so could just await the last one?
        requests.push(window._ipc.send('tcpSend', params))
      }

      try {
        await Promise.all(requests)
      } catch (err) {
        this.destroy(err)
        cb(err)
        return
      }

      cb()
    })()
  }

  _write (data, cb) {
    const params = {
      id: this.id,
      data
    }
    ;(async () => {
      const { err, data } = await window._ipc.send('tcpSend', params)
      console.log('_write', err, data)
      cb(err)
    })()
  }

  //
  // This is called internally by incoming _ipc message when there is data to insert to the stream.
  //
  __write (data) {
    if (data.length && !this.destroyed) {
      if (!this.push(data)) {
        const params = {
          id: this.id
        }
        this._flowing = false
        window._ipc.send('tcpReadStop', params)
      }
    } else {
      // if this stream is not full duplex,
      // then mark as not writable.
      if (!this.allowHalfOpen) {
        this.destroySoon()
      }
      this.push(null)
      this.read(0)
    }
  }

  _read (cb) {
    if (this._flowing) return cb()
    this._flowing = true

    const params = {
      id: this.id
    }

    ;(async () => {
      const { err } = await window._ipc.send('tcpReadStart', params)

      if (err) {
        this._destroy()
      } else {
        cb()
      }
    })()
  }

  pause () {
    Duplex.prototype.pause.call(this)
    // send a ReadStop but do not wait for a confirmation.
    // ipc is async, but it's ordered,
    if (this._flowing) {
      this._flowing = false
      window._ipc.send('tcpReadStop', { id: this.id })
    }
    return this
  }

  resume () {
    Duplex.prototype.resume.call(this)
    // send a ReadStop but do not wait for a confirmation.
    // ipc is async, but it's ordered,
    if (!this._flowing) {
      this._flowing = true
      window._ipc.send('tcpReadStart', { id: this.id })
    }
    return this
  }

  connect (...args) {
    const [options, cb] = normalizeArgs(args)

    ;(async () => {
      this.id = rand64()
      const params = {
        port: options.port,
        address: options.host,
        id: this.id
      }

      // TODO: if host is a ip address
      //      connect, if it is a dns name, lookup

      const { err, data } = await window._ipc.send('tcpConnect', params)

      if (err) {
        if (cb) cb(err)
        else this.emit('error', err)
        return
      }
      this.remotePort = data.port
      this.remoteAddress = data.address
      // this.port = port
      // this.address = address

      window._ipc.streams[this.id] = this

      if (cb) cb(null, this)
    })()
    return this
  }

  unref () {
    return this // for compatibility with the net module
  }
}

export const connect = (...args) => {
  const [options, callback] = normalizeArgs(args)

  // supported by node but not here: localAddress, localHost, hints, lookup

  const socket = new Socket(options)
  socket.connect(options, callback)

  return socket
}

export const createServer = (...args) => {
  return new Server(...args)
}

export const getNetworkInterfaces = o => window._ipc.send('os.networkInterfaces', o)

const v4Seg = '(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])'
const v4Str = `(${v4Seg}[.]){3}${v4Seg}`
const IPv4Reg = new RegExp(`^${v4Str}$`)

export const isIPv4 = s => {
  return IPv4Reg.test(s)
}
