import http from './http.js'

export const METHODS = http.METHODS
export const STATUS_CODES = http.STATUS_CODES

export class AgentOptions extends http.AgentOptions {}

export class Agent extends http.Agent {
  defaultProtocol = 'https:'
}

export class OutgoingMessage extends http.OutgoingMessage {}
export class ClientRequest extends http.ClientRequest {}
export class ServerResponse extends http.ServerResponse {}

export const globalAgent = new Agent()

export function request (optionsOrURL, options, callback) {
  if (typeof optionsOrURL === 'string') {
    options = { Agent, ...options }
    return http.request(optionsOrURL, options, callback)
  }

  options = { Agent, ...optionsOrURL }
  callback = options
  return http.request(optionsOrURL, options, callback)
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
