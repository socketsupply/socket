import { URLPattern } from './urlpattern/urlpattern.js'
import url from './url/url.js'

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

export const parse = parseURL

// lifted from node
export function resolve (from, to) {
  const resolved = new URL(to, new URL(from, 'resolve://'))

  if (resolved.protocol === 'resolve:') {
    const { pathname, search, hash } = resolved
    return pathname + search + hash
  }

  return resolved.toString()
}

url.serializeURLOrigin = function (input) {
  const { scheme, host } = input

  if (scheme === 'socket' || scheme === 'ipc') {
    return `${scheme}://${serializeHost(host)}`
  }

  return serializeURLOrigin(input)
}

export default URL
export { URLPattern, URL, URLSearchParams, parseURL }
