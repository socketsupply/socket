import { URLPattern } from './urlpattern/urlpattern.js'
import { URL } from './url.js'

export default URL
export { URLPattern, URL }

export class URLObject {
  protocol = ''
  username = ''
  password = ''
  hostname = ''
  port = ''
  pathname = ''
  search = ''
  hash = ''

  constructor (object) {
    Object.seal(this)
    object = parse(object)
    if (object) {
      for (const key in object) {
        const value = object[key]
        if (typeof value === 'string') {
          this[key] = value
        }
      }
    }
  }
}

export function toString (url) {
  if (!url) return ''
  if (typeof url === 'string') return url
  if (typeof url === 'object') {
    const {
      protocol = '',
      username = '',
      password = '',
      hostname = '',
      port = '',
      pathname = '',
      search = '',
      hash = ''
    } = url

    const components = []

    if (protocol) {
      const scheme = protocol.endsWith(':') ? protocol : `${protocol}:`
      components.push(scheme)
      components.push('//')
    }

    if (username) {
      components.push(username)

      if (password) components.push(':', password)

      components.push('@')
    }

    if (hostname) {
      components.push(hostname)
      if (port) components.push(port)
    }

    if (pathname) components.push(pathname)

    if (search || hash) {
      if (!pathname) components.push('/')
      if (search) components.push(search)
      if (hash) components.push(hash)
    }

    return components.join('')
  }

  return null
}

export function toObject (url) {
  if (!url) return null
  const parsed = typeof url === 'string' ? parse(url) : url
  const {
    protocol = '',
    username = '',
    password = '',
    hostname = '',
    port = '',
    pathname = '',
    search = '',
    hash = ''
  } = parsed

  return {
    protocol,
    username,
    password,
    hostname,
    port,
    pathname,
    search,
    hash
  }
}

/**
 * @param {string|object|URL|URLPattern}
 * @return {URLObject}
 */
export function parse (url) {
  if (typeof url !== 'string') {
    url = toString(url)
  }

  let pattern = null
  let didInjectProtocol = false

  try {
    pattern = new URLPattern(url)
  } catch {
    didInjectProtocol = true
    pattern = new URLPattern(`https://${url}`)
  }

  let {
    protocol = '',
    username = '',
    password = '',
    hostname = '',
    port = '',
    pathname = '',
    search = '',
    hash = ''
  } = (pattern || {})

  if (didInjectProtocol) {
    protocol = ''
  }

  return {
    protocol,
    username,
    password,
    hostname,
    port,
    pathname,
    search,
    hash
  }
}

/**
 * @param {string|URL|URLPattern} url
 * @param {string|URL|URLPattern} base
 * @return {string}
 */
export function resolve (url, base) {
  if (!url) {
    throw new TypeError(
      `Expecting first url argument to be a string: Received ${typeof url}`
    )
  }

  if (!base) {
    throw new TypeError(
      `Expecting second url argument to be a string: Received ${typeof base}`
    )
  }

  const baseURL = base ? parse(base) : null

  if (baseURL?.protocol) {
    return base
  }

  const sourceURL = parse(url)
  const isProtocolLess = !sourceURL?.protocol && !baseURL?.protocol
  let didInjectHostname = false

  if (!sourceURL.protocol) {
    sourceURL.protocol = baseURL?.protocol || 'https'
  }

  if (!sourceURL.hostname) {
    sourceURL.hostname = 'domain.com'
    didInjectHostname = true
  }

  const pattern = sourceURL && baseURL
    ? new URLPattern(base, toString(sourceURL))
    : new URLPattern(toString(sourceURL))

  const object = toObject(pattern)

  object.protocol = sourceURL.protocol

  if (didInjectHostname) {
    object.hostname = ''
    object.username = ''
    object.password = ''
    object.port = ''
  }

  if (isProtocolLess) {
    object.protocol = ''
  }

  return toString(object)
}
