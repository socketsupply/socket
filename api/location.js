export class Location {
  get url () {
    return globalThis.location.href.startsWith('blob:')
      ? new URL(globalThis.RUNTIME_WORKER_LOCATION || globalThis.location.pathname)
      : new URL(globalThis.location.href)
  }

  get protocol () {
    return 'socket:'
  }

  get host () {
    return this.url.host
  }

  get hostname () {
    return this.url.hostname
  }

  get port () {
    return this.url.port
  }

  get pathname () {
    return this.url.pathname
  }

  get search () {
    return this.url.search
  }

  get origin () {
    const origin = this.url.origin && this.url.origin !== 'null'
      ? this.url.origin
      : globalThis.origin || globalThis.location.origin

    return origin.replace('https://', 'socket://')
  }

  get href () {
    return this.url.href
  }

  get hash () {
    return this.url.hash
  }

  toString () {
    return this.href
  }
}

export default new Location()
