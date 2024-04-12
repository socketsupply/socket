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
    return this.url.origin
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
