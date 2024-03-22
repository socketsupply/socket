import http from './http.js'
// re-export
import * as exports from './http.js'

/**
 * All known possible HTTP methods.
 * @type {string[]}
 */
export const METHODS = http.METHODS

/**
 * A mapping of status codes to status texts
 * @type {object}
 */
export const STATUS_CODES = http.STATUS_CODES

/**
 * An options object container for an `Agent` instance.
 */
export class AgentOptions extends http.AgentOptions {}

/**
 * An Agent is responsible for managing connection persistence
 * and reuse for HTTPS clients.
 * @see {@link https://nodejs.org/api/https.html#class-httpsagent}
 */
export class Agent extends http.Agent {
  defaultProtocol = 'https:'
}

/**
 * An object that is created internally and returned from `request()`.
 * @see {@link https://nodejs.org/api/http.html#class-httpclientrequest}
 */
export class ClientRequest extends http.ClientRequest {}

/**
 * The parent class of `ClientRequest` and `ServerResponse`.
 * It is an abstract outgoing message from the perspective of the
 * participants of an HTTP transaction.
 * @see {@link https://nodejs.org/api/http.html#class-httpoutgoingmessage}
 */
export class OutgoingMessage extends http.OutgoingMessage {}

/**
 * An `IncomingMessage` object is created by `Server` or `ClientRequest` and
 * passed as the first argument to the 'request' and 'response' event
 * respectively.
 * It may be used to access response status, headers, and data.
 * @see {@link https://nodejs.org/api/http.html#class-httpincomingmessage}
 */
export class IncomingMessage extends http.IncomingMessage {}

/**
 * An object that is created internally by a `Server` instance, not by the user.
 * It is passed as the second parameter to the 'request' event.
 * @see {@link https://nodejs.org/api/http.html#class-httpserverresponse}
 */
export class ServerResponse extends http.ServerResponse {}

/**
 * A duplex stream between a HTTP request `IncomingMessage` and the
 * response `ServerResponse`
 */
export class Connection extends http.Connection {}

/**
 * A nodejs compat HTTP server typically intended for running in a "worker"
 * environment.
 * @see {@link https://nodejs.org/api/http.html#class-httpserver}
 */
export class Server extends http.Server {
  /**
   * The adapter interface for this `Server` instance.
   * @ignore
   */
  get adapterInterace () {
    return {
      Connection,
      globalAgent,
      IncomingMessage,
      METHODS,
      ServerResponse,
      STATUS_CODES
    }
  }
}

/**
 * The global and default HTTPS agent.
 * @type {Agent}
 */
export const globalAgent = new Agent()

/**
 * Makes a HTTPS request, optionally a `socket://` for relative paths when
 * `socket:` is the origin protocol.
 * @param {string|object} optionsOrURL
 * @param {(object|function)=} [options]
 * @param {function=} [callback]
 * @return {ClientRequest}
 */
export function request (optionsOrURL, options, callback) {
  if (typeof optionsOrURL === 'string') {
    options = { Agent, ...options }
    return http.request(optionsOrURL, options, callback)
  }

  options = { Agent, ...optionsOrURL }
  callback = options
  return http.request(optionsOrURL, options, callback)
}

/**
 * Makes a HTTPS or `socket:` GET request. A simplified alias to `request()`.
 * @param {string|object} optionsOrURL
 * @param {(object|function)=} [options]
 * @param {function=} [callback]
 * @return {ClientRequest}
 */
export function get (optionsOrURL, options, callback) {
  return request(optionsOrURL, options, callback)
}

/**
 * Creates a HTTPS server that can listen for incoming requests.
 * Requests that are dispatched to this server depend on the context
 * in which it is created, such as a service worker which will use a
 * "fetch event" adapter.
 * @param {object|function=} [options]
 * @param {function=} [callback]
 * @return {Server}
 */
export function createServer (...args) {
  return http.createServer(...args)
}

export default exports
