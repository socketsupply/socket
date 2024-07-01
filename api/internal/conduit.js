/**
 * @class Conduit
 *
 * @classdesc A class for managing WebSocket connections with custom options and payload encoding.
 */
export class Conduit {
  isActive = false
  id = null

  /**
   * Creates an instance of Conduit.
   *
   * @param {Object} params - The parameters for the Conduit.
   * @param {string} params.id - The ID for the connection.
   * @param {string} params.method - The method to use for the connection.
   */
  constructor ({ id }) {
    const port = globalThis.__args.conduit
    const clientId = globalThis.__args.client.top.id
    const uri = `ws://localhost:${port}/${id}/${clientId}`
    this.socket = new globalThis.WebSocket(uri)
    this.socket.binaryType = 'arraybuffer'
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
   * @param {Object} options - The options to encode.
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
   * @returns {Object} The decoded message containing options and payload.
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
   * The callback will receive an error object and an object containing decoded options and payload.
   *
   * @param {Function} cb - The callback function to handle incoming messages.
   * @param {Error} cb.error - The error object, if an error occurs. Null if no error.
   * @param {Object} cb.message - The decoded message object.
   * @param {Object} cb.message.options - The decoded options as key-value pairs.
   * @param {Uint8Array} cb.message.payload - The actual data of the payload.
   */
  receive (cb) {
    this.socket.onerror = err => {
      cb(err)
    }

    this.socket.onmessage = event => {
      const arrayBuffer = event.data
      const data = new Uint8Array(arrayBuffer)
      cb(null, this.decodeMessage(data))
    }
  }

  /**
   * Sends a message with the specified options and payload over the WebSocket connection.
   *
   * @param {Object} options - The options to send.
   * @param {Uint8Array} payload - The payload to send.
   */
  send (options, payload) {
    this.socket.send(this.encodeMessage(options, payload))
  }
}
