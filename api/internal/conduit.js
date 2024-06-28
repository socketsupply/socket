/**
 * @class Conduit
 *
 * @classdesc A class for managing WebSocket connections with custom headers and payload encoding.
 */
export class Conduit {
  /**
   * Creates an instance of Conduit.
   *
   * @param {Object} params - The parameters for the Conduit.
   * @param {string} params.id - The ID for the connection.
   * @param {string} params.method - The method to use for the connection.
   */
  constructor ({ id, method }) {
    const port = globalThis.__args.conduit
    const uri = `ws://localhost:${port}/${method}?id=${id}`
    this.socket = new globalThis.WebSocket(uri)
  }

  /**
   * Encodes a single header into a Uint8Array.
   *
   * @private
   * @param {string} key - The header key.
   * @param {string|number} value - The header value.
   * @returns {Uint8Array} The encoded header.
   * @throws Will throw an error if the header value type is invalid.
   */
  encodeHeader (key, value) {
    const keyLength = key.length
    const keyBuffer = new TextEncoder().encode(key)

    let valueBuffer
    let valueType

    if (typeof value === 'number') {
      valueType = 0
      valueBuffer = new TextEncoder().encode(value.toString())
    } else if (typeof value === 'string') {
      valueType = 1
      valueBuffer = new TextEncoder().encode(value)
    } else {
      throw new Error('Invalid header value type')
    }

    const valueLength = valueBuffer.length

    const buffer = new ArrayBuffer(1 + keyLength + 1 + 2 + valueLength)
    const view = new DataView(buffer)

    view.setUint8(0, keyLength)
    new Uint8Array(buffer, 1, keyLength).set(keyBuffer)

    view.setUint8(1 + keyLength, valueType)
    view.setUint16(2 + keyLength, valueLength, false)
    new Uint8Array(buffer, 4 + keyLength, valueLength).set(valueBuffer)

    return new Uint8Array(buffer)
  }

  /**
   * Encodes headers and payload into a single Uint8Array message.
   *
   * @private
   * @param {Object} headers - The headers to encode.
   * @param {Uint8Array} payload - The payload to encode.
   * @returns {Uint8Array} The encoded message.
   */
  encodeMessage (headers, payload) {
    const headerBuffers = Object.entries(headers)
      .map(([key, value]) => this.encodeHeader(key, value))

    const totalHeaderLength = headerBuffers.reduce((sum, buf) => sum + buf.length, 0)
    const bodyLength = payload.length
    const buffer = new ArrayBuffer(1 + totalHeaderLength + 2 + bodyLength)
    const view = new DataView(buffer)

    view.setUint8(0, headerBuffers.length)

    let offset = 1

    headerBuffers.forEach(headerBuffer => {
      new Uint8Array(buffer, offset, headerBuffer.length).set(headerBuffer)
      offset += headerBuffer.length
    })

    view.setUint16(offset, bodyLength)
    offset += 2

    new Uint8Array(buffer, offset, bodyLength).set(payload)

    return new Uint8Array(buffer)
  }

  /**
   * Decodes a Uint8Array message into headers and payload.
   * @param {Uint8Array} data - The data to decode.
   * @returns {Object} The decoded message containing headers and payload.
   * @throws Will throw an error if the data is invalid.
   */
  decodeMessage (data) {
    const view = new DataView(data.buffer)
    const numHeaders = view.getUint8(0)

    let offset = 1
    const headers = {}

    for (let i = 0; i < numHeaders; i++) {
      const keyLength = view.getUint8(offset)
      offset += 1

      const key = new TextDecoder().decode(new Uint8Array(data.buffer, offset, keyLength))
      offset += keyLength

      const valueType = view.getUint8(offset)
      offset += 1

      const valueLength = view.getUint16(offset, false)
      offset += 2

      const valueBuffer = new Uint8Array(data.buffer, offset, valueLength)
      offset += valueLength

      let value
      if (valueType === 0) {
        value = parseInt(new TextDecoder().decode(valueBuffer), 10)
      } else if (valueType === 1) {
        value = new TextDecoder().decode(valueBuffer)
      } else {
        throw new Error('Invalid header value type')
      }

      headers[key] = value
    }

    const bodyLength = view.getUint16(offset, false)
    offset += 2

    const payload = new Uint8Array(data.buffer, offset, bodyLength)

    return { headers, payload }
  }

  /**
   * Registers a callback to handle incoming messages.
   * The callback will receive an object containing decoded headers and payload.
   *
   * @param {Function} cb - The callback function to handle incoming messages.
   *   The callback receives a single parameter:
   * @param {Object} cb.message - The decoded message object.
   * @param {Object} cb.message.headers - The decoded headers as key-value pairs.
   * @param {Uint8Array} cb.message.payload - The actual data of the payload.
   */

  receive (cb) {
    this.socket.onmessage = event => this.decodeMessage(data)
  }

  /**
   * Sends a message with the specified headers and payload over the WebSocket connection.
   *
   * @param {Object} headers - The headers to send.
   * @param {Uint8Array} payload - The payload to send.
   */
  send (headers, payload) {
    this.socket.send(this.encodeMessage(headers, payload))
  }
}
