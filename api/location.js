export const globalLocation = globalThis.location ?? {
  origin: 'socket:///',
  host: '',
  hostname: '',
  pathname: '/',
  href: ''
}

export const href = globalLocation.href.replace(/https?:/, 'socket:')
export const protocol = 'socket:'
export const hostname = (
  // eslint-disable-next-line
  href.match(/^[a-zA-Z]+:[\/]{2}?(.*)(\/.*)$/) ?? []
)[1] ?? ''

export const host = hostname
export const search = href.split('?')[1] ?? ''
export const hash = href.split('#')[1] ?? ''
export const pathname = href.slice(
  href.indexOf(hostname) + hostname.length
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
