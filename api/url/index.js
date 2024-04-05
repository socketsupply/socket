import { URLPattern as URLPatternImplementation } from './urlpattern/urlpattern.js'
import url from './url/url.js'
import qs from '../querystring.js'

const {
  URL,
  URLSearchParams,
  parseURL,
  serializeURLOrigin,
  serializeHost
} = url

for (const key in globalThis.URL) {
  if (!URL[key]) {
    URL[key] = globalThis.URL[key].bind(globalThis.URL)
  }
}

URL.resolve = resolve
URL.parse = parse
URL.format = format

export class URLPattern extends URLPatternImplementation {}

const URLPatternDescriptors = Object.getOwnPropertyDescriptors(URLPattern.prototype)
Object.defineProperties(URLPatternDescriptors, {
  hash: { ...URLPatternDescriptors.hash, enumerable: true },
  hostname: { ...URLPatternDescriptors.hostname, enumerable: true },
  password: { ...URLPatternDescriptors.password, enumerable: true },
  pathname: { ...URLPatternDescriptors.pathname, enumerable: true },
  protocol: { ...URLPatternDescriptors.protocol, enumerable: true },
  username: { ...URLPatternDescriptors.username, enumerable: true },
  search: { ...URLPatternDescriptors.search, enumerable: true }
})

URL.prototype[Symbol.for('socket.runtime.util.inspect.custom')] = function () {
  return [
    'URL {',
    `  protocol: ${this.protocol || null},`,
    `  username: ${this.username || null},`,
    `  password: ${this.password || null},`,
    `  hostname: ${this.hostname || null},`,
    `  pathname: ${this.pathname || null},`,
    `  search: ${this.search || null},`,
    `  hash: ${this.hash || null}`,
    '}'
  ].join('\n')
}

export const protocols = new Set([
  'socket:',
  'node:',
  'npm:',
  'ipc:',

  // web standard & reserved
  'bitcoin:',
  'file:',
  'ftp:',
  'ftps:',
  'geo:',
  'git:',
  'http:',
  'https:',
  'im:',
  'ipfs:',
  'irc:',
  'ircs:',
  'magnet:',
  'mailto:',
  'matrix:',
  'mms:',
  'news:',
  'nntp:',
  'openpgp4fpr:',
  'sftp:',
  'sip:',
  'sms:',
  'smsto:',
  'ssh:',
  'tel:',
  'urn:',
  'webcal:',
  'wtai:',
  'xmpp:'
])

if (globalThis.__args?.config && typeof globalThis.__args.config === 'object') {
  const protocolHandlers = String(globalThis.__args.config['webview_protocol-handlers'] || '')
    .split(' ')
    .filter(Boolean)

  const webviewURLProtocols = String(globalThis.__args.config.webview_url_protocols || '')
    .split(' ')
    .filter(Boolean)

  for (const value of webviewURLProtocols) {
    const scheme = value.replace(':', '')
    if (scheme) {
      protocols.add(scheme + ':')
    }
  }

  for (const value of protocolHandlers) {
    const scheme = value.replace(':', '')
    if (scheme) {
      protocols.add(scheme + ':')
    }
  }

  for (const key in globalThis.__args.config) {
    if (key.startsWith('webview_protocol-handlers_')) {
      const scheme = key.replace('webview_protocol-handlers_', '').replace(':', '')
      if (scheme) {
        protocols.add(scheme + ':')
      }
    }
  }
}

export function parse (input, options = null) {
  let parsed = null
  if (URL.canParse(input)) {
    parsed = new URL(input)
  }

  if (options?.strict === true && !URL.canParse(input)) {
    return null
  }

  if (URL.canParse(input, 'socket://')) {
    parsed = new URL(input, globalThis.location.origin)
  }

  if (!parsed) {
    return null
  }

  parsed = {
    hash: parsed.hash || null,
    host: parsed.hostname || null,
    hostname: parsed.hostname || null,
    origin: parsed.origin || null,
    auth: [parsed.username, parsed.password].filter(Boolean).join(':') || null,
    password: parsed.password || null,
    pathname: parsed.pathname || null,
    path: parsed.pathname || null,
    port: parsed.port || null,
    protocol: parsed.protocol || null,
    search: parsed.search || null,
    searchParams: parsed.searchParams,
    username: parsed.username || null,
    [Symbol.toStringTag]: 'URL (Parsed)'
  }

  if (options === true) {
    // for nodejs compat
    parsed.query = Object.fromEntries(parsed.searchParams.entries())
  } else if (parsed.search) {
    parsed.query = parsed.search.slice(1) ?? null
  }

  if (!input.startsWith(parsed.protocol)) {
    parsed.protocol = null
    parsed.hostname = null
    parsed.origin = null
    parsed.host = null
    parsed.href = `${parsed.pathname || ''}${parsed.search || ''}${parsed.hash || ''}`
  } else {
    parsed.href = `${parsed.protocol}//`

    if (parsed.username) {
      parsed.href += [parsed.username, parsed.password].filter(Boolean).join(':')

      if (parsed.hostname) {
        parsed.href += '@'
      }
    }

    parsed.href += `${parsed.hostname || ''}${parsed.pathname || ''}${parsed.search || ''}${parsed.hash || ''}`
  }

  return parsed
}

// lifted from node
export function resolve (from, to) {
  const resolved = new URL(to, new URL(from, 'resolve://'))

  if (resolved.protocol === 'resolve:') {
    const { pathname, search, hash } = resolved
    return pathname + search + hash
  }

  return resolved.toString()
}

export function format (input) {
  if (!input || (typeof input !== 'string' && typeof input !== 'object')) {
    throw new TypeError(
      `The 'input' argument must be one of type object or string. Received: ${input}`
    )
  }

  if (typeof input === 'string') {
    if (!URL.canParse(input)) {
      return ''
    }

    return new URL(input).toString()
  }

  let formatted = ''

  if (!input.hostname) {
    return ''
  }

  if (input.protocol) {
    formatted += `${input.protocol}//`
  }

  if (input.username) {
    formatted += encodeURIComponent(input.username)

    if (input.password) {
      formatted += `:${encodeURIComponent(input.password)}`
    }

    formatted += '@'
  }

  formatted += input.hostname

  if (input.port) {
    formatted += `:${input.port}`
  }

  if (input.pathname) {
    formatted += input.pathname
  }

  if (input.query && typeof input.query === 'object') {
    formatted += `?${qs.stringify(input.query)}`
  } else if (input.query && typeof input.query === 'string') {
    if (!input.query.startsWith('?')) {
      formatted += '?'
    }

    formatted += encodeURIComponent(decodeURIComponent(input.query))
  }

  if (input.hash && typeof input.hash === 'string') {
    if (!input.hash.startsWith('#')) {
      formatted += '#'
    }

    formatted += input.hash
  }

  return formatted
}

url.serializeURLOrigin = function (input) {
  const { scheme, protocol, host } = input

  if (globalThis.__args.config.meta_application_protocol) {
    if (
      protocol &&
      globalThis.__args.config.meta_application_protocol === protocol.slice(0, -1)
    ) {
      return `${protocol}//${serializeHost(host)}`
    }

    if (
      scheme &&
      globalThis.__args.config.meta_application_protocol === scheme
    ) {
      return `${scheme}://${serializeHost(host)}`
    }
  }

  if (protocols.has(protocol)) {
    return `${protocol}//${serializeHost(host)}`
  }

  if (protocols.has(`${scheme}:`)) {
    return `${scheme}://${serializeHost(host)}`
  }

  return serializeURLOrigin(input)
}

const descriptors = Object.getOwnPropertyDescriptors(URL.prototype)
Object.defineProperties(URL.prototype, {
  ...descriptors,
  origin: {
    ...descriptors.origin,
    get () { return url.serializeURLOrigin(this) }
  }
})

export default URL
export { URL, URLSearchParams, parseURL }
