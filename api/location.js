export class Location {
  get url () {
    if (globalThis.location === this) {
      return null
    }

    if (globalThis.location.href.startsWith('blob:')) {
      return new URL(globalThis.RUNTIME_WORKER_LOCATION || globalThis.location.pathname)
    }

    if (globalThis.location.origin === 'null') {
      return new URL(
        globalThis.location.pathname +
        globalThis.location.search +
        globalThis.location.hash,
        globalThis.__args?.config?.meta_bundle_identifier ?? 'null'
      )
    }

    return new URL(globalThis.location.href)
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
