/* global CloseEvent, ErrorEvent, MessageEvent, WebSocket */
import client from '../application/client.js'
import ipc from '../ipc.js'

export const DEFALUT_MAX_RECONNECT_RETRIES = 32
export const DEFAULT_MAX_RECONNECT_TIMEOUT = 256

let defaultConduitPort = globalThis.__args.conduit

/**
 * @typedef {{ options: object, payload: Uint8Array }} ReceiveMessage
 * @typedef {function(Error?, ReceiveCallback | undefined)} ReceiveCallback
 * @typedef {{ id?: string|BigInt|number, reconnect?: {} }} ConduitOptions
 */

/**
 * @class Conduit
 * @ignore
 *
 * @classdesc A class for managing WebSocket connections with custom options and payload encoding.
 */
export class Conduit extends EventTarget {
  /**
   * The global `Conduit` port
   * @type {number}
   */
  static get port () { return defaultConduitPort }
  static set port (port) {
    if (port && typeof port === 'number') {
      defaultConduitPort = port
    }
  }

  /**
   * @type {boolean}
   */
  shouldReconnect = true

  /**
   * @type {boolean}
   */
  isConnecting = false

  /**
   * @type {boolean}
   */
  isActive = false

  /**
   * @type {WebSocket?}
   */
  socket = null

  /**
   * @type {number}
   */
  port = 0

  /**
   * @type {number?}
   */
  id = null

  /**
   * @private
   * @type {number}
   */
  #loop = 0

  /**
   * @private
   * @type {function(MessageEvent)}
   */
  #onmessage = null

  /**
   * @private
   * @type {function(CloseEvent)}
   */
  #onclose = null

  /**
   * @private
   * @type {function(ErrorEvent)}
   */
  #onerror = null

  /**
   * @type {function(Event)}
   */
  #onopen = null

  /**
   * Creates an instance of Conduit.
   *
   * @param {object} params - The parameters for the Conduit.
   * @param {string} params.id - The ID for the connection.
   * @param {string} params.method - The method to use for the connection.
   */
  constructor ({ id }) {
    super()

    this.id = id
    // @ts-ignore
    this.port = this.constructor.port
    this.connect()

    const reconnectState = {
      // TODO(@jwerle): eventually consume from 'options' when it exists
      retries: DEFALUT_MAX_RECONNECT_RETRIES
    }

    this.#loop = setInterval(() => {
      if (!this.isActive && !this.isConnecting && this.shouldReconnect) {
        this.reconnect({
          retries: --reconnectState.retries
        })
      }
    }, 256)
  }

  /**
   * The URL string for the WebSocket server.
   * @type {string}
   */
  get url () {
    return `ws://localhost:${this.port}/${this.id}/${client.top.id}`
  }

  /**
   * @type {function(MessageEvent)}
   */
  get onmessage () { return this.#onmessage }
  set onmessage (onmessage) {
    if (typeof this.#onmessage === 'function') {
      this.removeEventListener('message', this.#onmessage)
    }

    this.#onmessage = null

    if (typeof onmessage === 'function') {
      this.#onmessage = onmessage
      this.addEventListener('message', onmessage)
    }
  }

  /**
   * @type {function(ErrorEvent)}
   */
  get onerror () { return this.#onerror }
  set onerror (onerror) {
    if (typeof this.#onerror === 'function') {
      this.removeEventListener('error', this.#onerror)
    }

    this.#onerror = null

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  /**
   * @type {function(CloseEvent)}
   */
  get onclose () { return this.#onclose }
  set onclose (onclose) {
    if (typeof this.#onclose === 'function') {
      this.removeEventListener('close', this.#onclose)
    }

    this.#onclose = null

    if (typeof onclose === 'function') {
      this.#onclose = onclose
      this.addEventListener('close', onclose)
    }
  }

  /**
   * @type {function(Event)}
   */
  get onopen () { return this.#onopen }
  set onopen (onopen) {
    if (typeof this.#onopen === 'function') {
      this.removeEventListener('open', this.#onopen)
    }

    this.#onopen = null

    if (typeof onopen === 'function') {
      this.#onopen = onopen
      this.addEventListener('open', onopen)
    }
  }

  /**
   * Connects the underlying conduit `WebSocket`.
   * @param {function(Error?)=} [callback]
   * @return {Promise<Conduit>}
   */
  async connect (callback = null) {
    if (this.isConnecting) {
      return this
    }

    if (this.socket) {
      this.socket.close()
    }

    // reset
    this.isActive = false
    this.isConnecting = true

    // @ts-ignore
    const resolvers = Promise.withResolvers()
    const result = await ipc.send('internal.conduit.start')

    if (result.err) {
      if (typeof callback === 'function') {
        callback(result.err)
        return this
      } else {
        throw result.err
      }
    }

    this.port = result.data.port
    this.socket = new WebSocket(this.url)
    this.socket.binaryType = 'arraybuffer'
    this.socket.onerror = (e) => {
      this.isActive = false
      this.isConnecting = false
      this.dispatchEvent(new ErrorEvent('error', e))

      if (typeof callback === 'function') {
        callback(e.error || new Error('Failed to connect Conduit'))
        callback = null
      }

      resolvers.reject(e.error ?? new Error())
    }

    this.socket.onclose = (e) => {
      this.isActive = false
      this.isConnecting = false
      this.dispatchEvent(new CloseEvent('close', e))
    }

    this.socket.onopen = (e) => {
      this.isActive = true
      this.isConnecting = false
      this.dispatchEvent(new Event('open'))

      if (typeof callback === 'function') {
        callback(null)
        callback = null
      }

      resolvers.resolve()
    }

    this.socket.onmessage = (e) => {
      this.isActive = true
      this.isConnecting = false
      this.dispatchEvent(new MessageEvent('message', e))
    }

    await resolvers.promise
    return this
  }

  /**
   * Reconnects a `Conduit` socket.
   * @param {{retries?: number, timeout?: number}} [options]
   * @return {Promise<Conduit>}
   */
  async reconnect (options = null) {
    if (this.isConnecting) {
      return this
    }

    const retries = options?.retries ?? DEFALUT_MAX_RECONNECT_RETRIES
    const timeout = options?.timeout ?? DEFAULT_MAX_RECONNECT_TIMEOUT

    return await this.connect((err) => {
      if (err) {
        this.isActive = false
        if (retries > 0) {
          setTimeout(() => this.reconnect({
            retries: retries - 1,
            timeout
          }), timeout)
        }
      }
    })
  }

  /**
   * Encodes a single header into a Uint8Array.
   *
   * @private
   * @param {string} key - The header key.
   * @param {string} value - The header value.
   * @returns {Uint8Array} The encoded header.
   */
  encodeOption (key, value) {
    const keyLength = key.length
    const keyBuffer = new TextEncoder().encode(key)

    const valueBuffer = new TextEncoder().encode(value)
    const valueLength = valueBuffer.length

    const buffer = new ArrayBuffer(1 + keyLength + 2 + valueLength)
    const view = new DataView(buffer)

    view.setUint8(0, keyLength)
    new Uint8Array(buffer, 1, keyLength).set(keyBuffer)

    view.setUint16(1 + keyLength, valueLength, false)
    new Uint8Array(buffer, 3 + keyLength, valueLength).set(valueBuffer)

    return new Uint8Array(buffer)
  }

  /**
   * Encodes options and payload into a single Uint8Array message.
   *
   * @private
   * @param {object} options - The options to encode.
   * @param {Uint8Array} payload - The payload to encode.
   * @returns {Uint8Array} The encoded message.
   */
  encodeMessage (options, payload) {
    const headerBuffers = Object.entries(options)
      .map(([key, value]) => this.encodeOption(key, value))

    const totalOptionLength = headerBuffers.reduce((sum, buf) => sum + buf.length, 0)
    const bodyLength = payload.length
    const buffer = new ArrayBuffer(1 + totalOptionLength + 2 + bodyLength)
    const view = new DataView(buffer)

    view.setUint8(0, headerBuffers.length)

    let offset = 1

    headerBuffers.forEach(headerBuffer => {
      new Uint8Array(buffer, offset, headerBuffer.length).set(headerBuffer)
      offset += headerBuffer.length
    })

    view.setUint16(offset, bodyLength, false)
    offset += 2

    new Uint8Array(buffer, offset, bodyLength).set(payload)

    return new Uint8Array(buffer)
  }

  /**
   * Decodes a Uint8Array message into options and payload.
   * @param {Uint8Array} data - The data to decode.
   * @return {ReceiveMessage} The decoded message containing options and payload.
   * @throws Will throw an error if the data is invalid.
   */
  decodeMessage (data) {
    const view = new DataView(data.buffer)
    const numOpts = view.getUint8(0)

    let offset = 1
    const options = {}

    for (let i = 0; i < numOpts; i++) {
      const keyLength = view.getUint8(offset)
      offset += 1

      const key = new TextDecoder().decode(new Uint8Array(data.buffer, offset, keyLength))
      offset += keyLength

      const valueLength = view.getUint16(offset, false)
      offset += 2

      const valueBuffer = new Uint8Array(data.buffer, offset, valueLength)
      offset += valueLength

      const value = new TextDecoder().decode(valueBuffer)
      options[key] = value
    }

    const bodyLength = view.getUint16(offset, false)
    offset += 2

    const payload = new Uint8Array(data.buffer, offset, bodyLength)
    return { options, payload }
  }

  /**
   * Registers a callback to handle incoming messages.
   * The callback will receive an error object and an object containing
   * decoded options and payload.
   * @param {ReceiveCallback} callback - The callback function to handle incoming messages.
   */
  receive (callback) {
    this.addEventListener('error', (event) => {
      // @ts-ignore
      callback(event.error || new Error())
    })

    this.addEventListener('message', (event) => {
      // @ts-ignore
      const data = new Uint8Array(event.data)
      callback(null, this.decodeMessage(data))
    })
  }

  /**
   * Sends a message with the specified options and payload over the
   * WebSocket connection.
   * @param {object} options - The options to send.
   * @param {Uint8Array} payload - The payload to send.
   * @return {boolean}
   */
  send (options, payload) {
    if (this.isActive) {
      this.socket.send(this.encodeMessage(options, payload))
      return true
    }

    return false
  }

  /**
   * Closes the WebSocket connection, preventing reconnects.
   */
  close () {
    this.shouldReconnect = false
    if (this.#loop) {
      clearInterval(this.#loop)
      this.#loop = 0
    }
    if (this.socket) {
      this.socket.close()
      this.socket = null
    }
  }
}
