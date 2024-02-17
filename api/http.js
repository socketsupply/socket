import { Writable, Duplex } from './stream.js'
import { EventEmitter } from './events.js'

export const METHODS = [
  'ACL',
  'BIND',
  'CHECKOUT',
  'CONNECT',
  'COPY',
  'DELETE',
  'GET',
  'HEAD',
  'LINK',
  'LOCK',
  'M-SEARCH',
  'MERGE',
  'MKACTIVITY',
  'MKCALENDAR',
  'MKCOL',
  'MOVE',
  'NOTIFY',
  'OPTIONS',
  'PATCH',
  'POST',
  'PROPFIND',
  'PROPPATCH',
  'PURGE',
  'PUT',
  'REBIND',
  'REPORT',
  'SEARCH',
  'SOURCE',
  'SUBSCRIBE',
  'TRACE',
  'UNBIND',
  'UNLINK',
  'UNLOCK',
  'UNSUBSCRIBE'
]

export const STATUS_CODES = {
  100: 'Continue',
  101: 'Switching Protocols',
  102: 'Processing',
  103: 'Early Hints',
  200: 'OK',
  201: 'Created',
  202: 'Accepted',
  203: 'Non-Authoritative Information',
  204: 'No Content',
  205: 'Reset Content',
  206: 'Partial Content',
  207: 'Multi-Status',
  208: 'Already Reported',
  226: 'IM Used',
  300: 'Multiple Choices',
  301: 'Moved Permanently',
  302: 'Found',
  303: 'See Other',
  304: 'Not Modified',
  305: 'Use Proxy',
  307: 'Temporary Redirect',
  308: 'Permanent Redirect',
  400: 'Bad Request',
  401: 'Unauthorized',
  402: 'Payment Required',
  403: 'Forbidden',
  404: 'Not Found',
  405: 'Method Not Allowed',
  406: 'Not Acceptable',
  407: 'Proxy Authentication Required',
  408: 'Request Timeout',
  409: 'Conflict',
  410: 'Gone',
  411: 'Length Required',
  412: 'Precondition Failed',
  413: 'Payload Too Large',
  414: 'URI Too Long',
  415: 'Unsupported Media Type',
  416: 'Range Not Satisfiable',
  417: 'Expectation Failed',
  418: "I'm a Teapot",
  421: 'Misdirected Request',
  422: 'Unprocessable Entity',
  423: 'Locked',
  424: 'Failed Dependency',
  425: 'Too Early',
  426: 'Upgrade Required',
  428: 'Precondition Required',
  429: 'Too Many Requests',
  431: 'Request Header Fields Too Large',
  451: 'Unavailable For Legal Reasons',
  500: 'Internal Server Error',
  501: 'Not Implemented',
  502: 'Bad Gateway',
  503: 'Service Unavailable',
  504: 'Gateway Timeout',
  505: 'HTTP Version Not Supported',
  506: 'Variant Also Negotiates',
  507: 'Insufficient Storage',
  508: 'Loop Detected',
  509: 'Bandwidth Limit Exceeded',
  510: 'Not Extended',
  511: 'Network Authentication Required'
}

export class OutgoingMessage extends Writable {
  headers = new Headers()
  socket = null

  get headersSent () {
    return true
  }

  appendHeader (name, value) {
    this.headers.set(name.toLowerCase(), value)
    return this
  }

  setHeader (name, value) {
    return this.appendHeader(name, value)
  }

  flushHeaders () {
    this.emit('flushheaders')
  }

  getHeader (name) {
    return this.headers.get(name.toLowerCase())
  }

  getHeaderNames () {
    return Array.from(this.headers.keys())
  }

  getHeaders () {
    return Object.fromEntries(this.headers.entries())
  }

  hasHeader (name) {
    return this.headers.has(name.toLowerCase())
  }

  removeHeader (name) {
    return this.headers.delete(name.toLowerCase())
  }
}

export class ClientRequest extends OutgoingMessage {}

export class ServerResponse extends OutgoingMessage {}

export class AgentOptions {
  keepAlive = false
  timeout = -1

  constructor (options) {
    this.keepAlive = options?.keepAlive === true
    this.timeout = Number.isFinite(options?.timeout) && options.timeout > 0
      ? options.timeout
      : -1
  }
}

export class Agent extends EventEmitter {
  defaultProtocol = 'http:'
  options = null

  constructor (options) {
    super()
    this.options = new AgentOptions(options)
  }

  createConnection (options, callback = null) {
    let controller = null
    let timeout = null
    let url = null

    const abortController = new AbortController()
    const readable = new ReadableStream({ start (c) { controller = c } })
    const pending = { callbacks: [], data: [] }

    const stream = new Duplex({
      signal: abortController.signal,
      write (data, cb) {
        console.log('write')
        controller.enqueue(data)
        cb(null)
      },

      read (cb) {
        if (pending.data.length) {
          const data = pending.data.shift()
          this.push(data)
          cb(null)
        } else {
          pending.callbacks.push(cb)
        }
      }
    })

    stream.on('finish', () => readable.close())

    url = `${options.protocol ?? this.defaultProtocol}//`
    url += (options.host || options.hostname)
    if (options.port) {
      url += `:${options.port}`
    }

    url += (options.path || options.pathname)

    if (options.signal) {
      options.signal.addEventListener('abort', () => {
        abortController.abort(options.signal.reason)
      })
    }

    if (options.timeout) {
      timeout = setTimeout(() => {
        abortController.abort('Connection timed out')
        stream.emit('timeout')
      }, options.timeout)
    }

    abortController.signal.addEventListener('abort', () => {
      stream.emit('aborted')
      stream.emit('error', Object.assign(new Error('aborted'), { code: 'ECONNRESET' }))
    })

    const deferredRequestPromise = options.makeRequest
      ? options.makeRequest()
      : Promise.resolve()

    deferredRequestPromise.then(makeRequest)

    function makeRequest (req) {
      const request = fetch(url, {
        headers: Object.fromEntries(
          Array.from(Object.entries(
            options.headers?.entries?.() ?? options.headers ?? {}
          )).concat(req.headers.entries())
        ),
        signal: abortController.signal,
        method: options.method ?? 'GET',
        body: /put|post/i.test(options.method ?? '')
          ? readable
          : undefined
      })

      if (options.handleResponse) {
        request.then(options.handleResponse)
      }

      request.finally(() => clearTimeout(timeout))
      request
        .then((response) => {
          if (response.body) {
            return response.body.getReader()
          }

          return response
            .blob()
            .then((blob) => blob.stream().getReader())
        })
        .then((reader) => {
          read()
          function read () {
            reader.read()
              .then(({ done, value }) => {
                if (pending.callbacks.length) {
                  const cb = pending.callbacks.shift()
                  stream.push(value)
                  cb(null)
                } else {
                  pending.data.push(value ?? null)
                }

                if (!done) {
                  read()
                }
              })
          }
        })

      if (typeof callback === 'function') {
        callback(stream)
      }
    }

    return stream
  }
}

export const globalAgent = new Agent()

async function request (optionsOrURL, options, callback) {
  if (optionsOrURL && typeof optionsOrURL === 'object') {
    options = optionsOrURL
    callback = options
  } else if (typeof optionsOrURL === 'string') {
    const url = globalThis.location.origin.startsWith('blob')
      ? new URL(optionsOrURL, new URL(globalThis.location.origin).pathname)
      : new URL(optionsOrURL, globalThis.location.origin)

    options = {
      host: url.host,
      port: url.port,
      pathname: url.pathname,
      protocol: url.protocol,
      ...options
    }
  }

  const request = new ClientRequest()
  let stream = null
  let agent = null

  options = {
    ...options,
    makeRequest () {
      return new Promise((resolve) => {
        if (!/post|put/i.test(options.method ?? '')) {
          resolve(request)
        } else {
          stream.on('finish', () => resolve(request))
        }

        request.emit('connect')
      })
    },

    handleResponse (response) {
      stream.response = response
      request.emit('response', stream)
    }
  }

  if (options.agent) {
    agent = options.agent
  } else if (options.agent === false) {
    agent = new (options.Agent ?? Agent)()
  } else {
    agent = globalAgent
  }

  stream = agent.createConnection(options, callback)

  stream.on('finish', () => request.emit('finish'))
  stream.on('timeout', () => request.emit('timeout'))

  return request
}

export function get (optionsOrURL, options, callback) {
  return request(optionsOrURL, options, callback)
}

export default {
  METHODS,
  STATUS_CODES,
  AgentOptions,
  Agent,
  globalAgent,
  request,
  OutgoingMessage,
  ClientRequest,
  ServerResponse,
  get
}
