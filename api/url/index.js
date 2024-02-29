import { URLPattern } from './urlpattern/urlpattern.js'
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

export const protocols = new Set([
  'socket:',
  'ipc:'
])

export function parse (input, options = null) {
  if (URL.canParse(input)) {
    return new URL(input)
  }

  if (options?.strict !== true && URL.canParse(input, 'socket://')) {
    return new URL(input, 'socket://')
  }

  return null
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
export { URLPattern, URL, URLSearchParams, parseURL }
