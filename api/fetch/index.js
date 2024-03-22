/**
 * @ignore
 */
import { fetch, Headers, Request, Response } from './fetch.js'
import { Deferred } from '../async/deferred.js'
import { Buffer } from '../buffer.js'

export {
  fetch,
  Headers,
  Request,
  Response
}

Response.json = function (json, options) {
  return new Response(JSON.stringify(json), {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...options?.headers
    }
  })
}

const { _initBody } = Response.prototype

Response.prototype._initBody = async function (body) {
  this.body = null
  if (typeof globalThis.ReadableStream === 'function') {
    if (body && body instanceof globalThis.ReadableStream) {
      let controller = null
      this.body = new ReadableStream({
        start (c) {
          controller = c
        }
      })

      const chunks = []
      const deferred = new Deferred()

      Object.assign(this, {
        async text () {
          await deferred
          return Response.prototype.text.call(this)
        },

        async blob () {
          await deferred
          return Response.prototype.blob.call(this)
        },

        async arrayBuffer () {
          await deferred
          return Response.prototype.arrayBuffer.call(this)
        }
      })

      for await (const chunk of body) {
        controller.enqueue(chunk)
        chunks.push(chunk)
      }

      const buffer = Buffer.concat(chunks)

      this._bodyArrayBuffer = buffer.buffer
      this._bodyInit = new Blob([this._bodyArrayBuffer])

      controller.close()
      deferred.resolve()

      return
    }
  }

  return _initBody.call(this, body)
}

Object.defineProperties(Request.prototype, {
  url: {
    configurable: true,
    writable: true,
    value: undefined
  },

  credentials: {
    configurable: true,
    writable: true,
    value: undefined
  },

  method: {
    configurable: true,
    writable: true,
    value: undefined
  },

  mode: {
    configurable: true,
    writable: true,
    value: undefined
  },

  signal: {
    configurable: true,
    writable: true,
    value: undefined
  },

  headers: {
    configurable: true,
    writable: true,
    value: undefined
  },

  referrer: {
    configurable: true,
    writable: true,
    value: undefined
  }
})

Object.defineProperties(Response.prototype, {
  body: {
    configurable: true,
    writable: true,
    value: undefined
  },

  type: {
    configurable: true,
    writable: true,
    value: undefined
  },

  status: {
    configurable: true,
    writable: true,
    value: undefined
  },

  statusText: {
    configurable: true,
    writable: true,
    value: undefined
  },

  ok: {
    configurable: true,
    writable: true,
    value: undefined
  },

  url: {
    configurable: true,
    writable: true,
    value: undefined
  },

  headers: {
    configurable: true,
    writable: true,
    value: undefined
  }
})

export default fetch
