export class Location {
  get url () {
    let url = null

    // XXX(@jwerle): should never be true...
    // @ts-ignore
    if (globalThis.location === this) {
      return url
    }

    if (globalThis.location.href.startsWith('blob:')) {
      url = new URL(
        globalThis.RUNTIME_WORKER_LOCATION || (
          globalThis.location.pathname +
          globalThis.location.search +
          globalThis.location.hash
        )
      )
    } else if (globalThis.location.origin === 'null') {
      try {
        url = new URL(
          globalThis.location.pathname +
          globalThis.location.search +
          globalThis.location.hash,
          globalThis.__args?.config?.meta_bundle_identifier ?? 'null'
        )
      } catch {}
    } else if (globalThis.location.hostname === globalThis.__args?.config?.meta_bundle_identifier) {
      url = new URL(globalThis.location.href)
    } else if (globalThis.__args.client.host === globalThis.location.hostname) {
      url = new URL(globalThis.location.href)
    } else if (globalThis.top !== globalThis) {
      return new URL(globalThis.location.href)
    }

    if (!url || url.hostname !== globalThis.__args?.config?.meta_bundle_identifier) {
      if (globalThis.__args?.config?.meta_bundle_identifier) {
        if (globalThis.__args.config.platform === 'android') {
          url = new URL(`https://${globalThis.__args.config.meta_bundle_identifier}`)
        } else {
          url = new URL(`socket://${globalThis.__args.config.meta_bundle_identifier}`)
        }
      }
    }

    return url
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
