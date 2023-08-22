export const globalLocation = globalThis.location ?? {
  origin: 'socket:///',
  host: '',
  hostname: '',
  pathname: '/',
  href: ''
}

export const href = globalLocation.href
export const protocol = 'socket:'
export const hostname = (
  // eslint-disable-next-line
  globalLocation.href.match(/^[a-zA-Z]+:[\/]{2}?(.*)(\/.*)$/) ?? []
)[1] ?? ''

export const host = hostname
export const search = globalLocation.href.split('?')[1] ?? ''
export const hash = globalLocation.href.split('#')[1] ?? ''
export const pathname = globalLocation.href.slice(
  globalLocation.href.indexOf(hostname) + hostname.length
)

export const origin = `${protocol}//${(host + pathname).replace(/\/\//g, '/')}`

export function toString () {
  return href
}

export default {
  origin,
  href,
  protocol,
  hostname,
  host,
  search,
  pathname,
  toString
}
