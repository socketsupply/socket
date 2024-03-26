import http from './http.js'
// re-export
import * as exports from './http.js'

export const CONTINUE = http.CONTINUE
export const SWITCHING_PROTOCOLS = http.SWITCHING_PROTOCOLS
export const PROCESSING = http.PROCESSING
export const EARLY_HINTS = http.EARLY_HINTS
export const OK = http.OK
export const CREATED = http.CREATED
export const ACCEPTED = http.ACCEPTED
export const NONAUTHORITATIVE_INFORMATION = http.NONAUTHORITATIVE_INFORMATION
export const NO_CONTENT = http.NO_CONTENT
export const RESET_CONTENT = http.RESET_CONTENT
export const PARTIAL_CONTENT = http.PARTIAL_CONTENT
export const MULTISTATUS = http.MULTISTATUS
export const ALREADY_REPORTED = http.ALREADY_REPORTED
export const IM_USED = http.IM_USED
export const MULTIPLE_CHOICES = http.MULTIPLE_CHOICES
export const MOVED_PERMANENTLY = http.MOVED_PERMANENTLY
export const FOUND = http.FOUND
export const SEE_OTHER = http.SEE_OTHER
export const NOT_MODIFIED = http.NOT_MODIFIED
export const USE_PROXY = http.USE_PROXY
export const TEMPORARY_REDIRECT = http.TEMPORARY_REDIRECT
export const PERMANENT_REDIRECT = http.PERMANENT_REDIRECT
export const BAD_REQUEST = http.BAD_REQUEST
export const UNAUTHORIZED = http.UNAUTHORIZED
export const PAYMENT_REQUIRED = http.PAYMENT_REQUIRED
export const FORBIDDEN = http.FORBIDDEN
export const NOT_FOUND = http.NOT_FOUND
export const METHOD_NOT_ALLOWED = http.METHOD_NOT_ALLOWED
export const NOT_ACCEPTABLE = http.NOT_ACCEPTABLE
export const PROXY_AUTHENTICATION_REQUIRED = http.PROXY_AUTHENTICATION_REQUIRED
export const REQUEST_TIMEOUT = http.REQUEST_TIMEOUT
export const CONFLICT = http.CONFLICT
export const GONE = http.GONE
export const LENGTH_REQUIRED = http.LENGTH_REQUIRED
export const PRECONDITION_FAILED = http.PRECONDITION_FAILED
export const PAYLOAD_TOO_LARGE = http.PAYLOAD_TOO_LARGE
export const URI_TOO_LONG = http.URI_TOO_LONG
export const UNSUPPORTED_MEDIA_TYPE = http.UNSUPPORTED_MEDIA_TYPE
export const RANGE_NOT_SATISFIABLE = http.RANGE_NOT_SATISFIABLE
export const EXPECTATION_FAILED = http.EXPECTATION_FAILED
export const IM_A_TEAPOT = http.IM_A_TEAPOT
export const MISDIRECTED_REQUEST = http.MISDIRECTED_REQUEST
export const UNPROCESSABLE_ENTITY = http.UNPROCESSABLE_ENTITY
export const LOCKED = http.LOCKED
export const FAILED_DEPENDENCY = http.FAILED_DEPENDENCY
export const TOO_EARLY = http.TOO_EARLY
export const UPGRADE_REQUIRED = http.UPGRADE_REQUIRED
export const PRECONDITION_REQUIRED = http.PRECONDITION_REQUIRED
export const TOO_MANY_REQUESTS = http.TOO_MANY_REQUESTS
export const REQUEST_HEADER_FIELDS_TOO_LARGE = http.REQUEST_HEADER_FIELDS_TOO_LARGE
export const UNAVAILABLE_FOR_LEGAL_REASONS = http.UNAVAILABLE_FOR_LEGAL_REASONS
export const INTERNAL_SERVER_ERROR = http.INTERNAL_SERVER_ERROR
export const NOT_IMPLEMENTED = http.NOT_IMPLEMENTED
export const BAD_GATEWAY = http.BAD_GATEWAY
export const SERVICE_UNAVAILABLE = http.SERVICE_UNAVAILABLE
export const GATEWAY_TIMEOUT = http.GATEWAY_TIMEOUT
export const HTTP_VERSION_NOT_SUPPORTED = http.HTTP_VERSION_NOT_SUPPORTED
export const VARIANT_ALSO_NEGOTIATES = http.VARIANT_ALSO_NEGOTIATES
export const INSUFFICIENT_STORAGE = http.INSUFFICIENT_STORAGE
export const LOOP_DETECTED = http.LOOP_DETECTED
export const BANDWIDTH_LIMIT_EXCEEDED = http.BANDWIDTH_LIMIT_EXCEEDED
export const NOT_EXTENDED = http.NOT_EXTENDED
export const NETWORK_AUTHENTICATION_REQUIRED = http.NETWORK_AUTHENTICATION_REQUIRED

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
