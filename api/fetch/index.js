/**
 * @ignore
 */
import { fetch, Headers, Request, Response } from './fetch.js'
import { Deferred } from '../async/deferred.js'
import { Buffer } from '../buffer.js'
import http from '../http.js'

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

const initResponseBody = Response.prototype._initBody
const initRequestBody = Request.prototype._initBody
const textEncoder = new TextEncoder()

Response.prototype._initBody = initBody
Request.prototype._initBody = initBody

async function initBody (body, options, xhr) {
  this.body = null

  if (
    typeof this.statusText !== 'string' &&
    Number.isFinite(this.status) &&
    this.status in http.STATUS_CODES
  ) {
    this.statusText = http.STATUS_CODES[this.status]
  }

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
        chunks.push(new Uint8Array(chunk))
      }

      const buffer = Buffer.concat(chunks)

      this._bodyArrayBuffer = buffer.buffer
      this._bodyInit = new Blob([this._bodyArrayBuffer])

      controller.close()
      deferred.resolve()

      return
    }
  }

  if (this instanceof Request) {
    initRequestBody.call(this, body)
  } else if (this instanceof Response) {
    initResponseBody.call(this, body)
  }

  if (!this.body && !this._noBody) {
    if (this._bodyArrayBuffer) {
      const arrayBuffer = this._bodyArrayBuffer
      this.body = new ReadableStream({
        start (controller) {
          controller.enqueue(arrayBuffer)
          controller.close()
        }
      })
    } else if (this._bodyBlob) {
      const blob = this._bodyBlob
      this.body = new ReadableStream({
        async start (controller) {
          controller.enqueue(await blob.arrayBuffer())
          controller.close()
        }
      })
    } else if (this._bodyText) {
      const text = this._bodyText
      this.body = new ReadableStream({
        start (controller) {
          controller.enqueue(textEncoder.encode(text))
          controller.close()
        }
      })
    } else {
      this.body = null
    }
  }

  if (this instanceof Request) {
    options.onxhr = (xhr) => {
      const accept = this.headers.get('accept') || ''
      if (accept.startsWith('text/event-stream')) {
        try {
          xhr.responseType = 'text'
        } catch {}
      }
    }
  }

  const contentType = this.headers.get('content-type') || ''
  if (xhr && contentType.startsWith('text/event-stream')) {
    let controller
    let byteOffset = 0
    this.body = new ReadableStream({
      type: 'bytes',
      start (c) {
        controller = c
      }
    })
    xhr.addEventListener('load', () => {
      while (controller && byteOffset < xhr.responseText.length) {
        xhr.onprogress()
      }

      if (controller) {
        controller.close()
        controller = null
      }
    }, { once: true })
    xhr.onprogress = () => {
      try {
        if (controller?.byobRequest) {
          const byteLength = controller.byobRequest.view.byteLength
          const bytesRead = Math.min(byteLength, xhr.responseText.length - byteOffset)
          const text = xhr.responseText.slice(byteOffset, byteOffset + bytesRead)
          const buffer = textEncoder.encode(text)

          if (bytesRead === 0) {
            controller.close()
            controller = null
          } else {
            byteOffset += bytesRead
            controller.byobRequest.view.set(buffer)
            controller.byobRequest.respond(bytesRead)
          }
        } else if (controller) {
          const bytesRead = xhr.responseText.length - byteOffset
          const text = xhr.responseText.slice(byteOffset, byteOffset + bytesRead)
          const buffer = textEncoder.encode(text)
          byteOffset += bytesRead
          controller.enqueue(buffer)
        }
      } catch {}
    }
  }
}

Object.defineProperties(Request.prototype, {
  _bodyArrayBuffer: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyFormData: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyText: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyInit: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyBlob: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _noBody: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  url: {
    configurable: true,
    writable: true,
    value: undefined
  },

  body: {
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
  _bodyArrayBuffer: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyFormData: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyText: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyInit: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _bodyBlob: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

  _noBody: {
    configurable: true,
    enumerable: false,
    writable: true,
    value: undefined
  },

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
  },

  redirected: {
    configurable: true,
    writable: true,
    value: undefined
  }
})

export default fetch
