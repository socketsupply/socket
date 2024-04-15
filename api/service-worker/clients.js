import application from '../application.js'
import state from './state.js'

const MAX_WINDOWS = 32
const CLIENT_GET_TIMEOUT = 100
const CLIENT_MATCH_ALL_TIMEOUT = 50

function normalizeURL (url) {
  if (typeof url !== 'string') {
    url = url.toString()
  }

  if (!URL.canParse(url) && !url.startsWith('/') && !url.startsWith('.')) {
    url = `./${url}`
  }

  return new URL(url, globalThis.location.origin)
}

function getOrigin () {
  if (globalThis.location.origin.startsWith('blob:')) {
    return new URL(globalThis.location.origin).pathname
  }

  return globalThis.location.origin
}

export class Client {
  #id = null
  #url = null
  #type = null
  #frameType = null

  constructor (options) {
    this.#id = options?.id ?? null
    this.#url = options?.url ?? null
    this.#type = options?.type ?? null
    this.#frameType = options?.frameType ?? null
  }

  get id () {
    return this.#id
  }

  get url () {
    return this.#url
  }

  get type () {
    return this.#type ?? 'none'
  }

  get frameType () {
    return this.#frameType ?? 'none'
  }

  postMessage (message, optionsOrTransferables = null) {
    globalThis.postMessage({
      from: 'serviceWorker',
      registration: { id: state.id },
      client: {
        id: this.#id,
        type: this.#type,
        frameType: this.#frameType
      },
      message
    }, optionsOrTransferables)
  }
}

export class WindowClient extends Client {
  #url = null
  #window = null
  #focused = false
  #ancestorOrigins = []
  #visibilityState = 'prerender'

  constructor (options) {
    super({
      ...options,
      id: options?.window?.id
    })

    this.#url = options?.url ?? null
    this.#window = options?.window ?? null

    state.channel.addEventListener('message', (event) => {
      if (event.data?.client?.id === this.id) {
        if ('focused' in event.data.client) {
          this.#focused = Boolean(event.data.client.focused)
        }

        if ('visibilityState' in event.data.client) {
          this.#visibilityState = event.data.client.visibilityState
        }
      }
    })
  }

  get url () {
    return this.#url
  }

  get focused () {
    return this.#focused
  }

  get ancestorOrigins () {
    return this.#ancestorOrigins
  }

  get visibilityState () {
    return this.#visibilityState
  }

  async focus () {
    state.channel.postMessage({
      client: {
        id: this.id,
        focus: true
      }
    })

    return this
  }

  async navigate (url) {
    const origin = getOrigin()

    url = normalizeURL(url)

    if (!url.toString().startsWith(origin)) {
      throw new TypeError('WindowClient cannot navigate outside of origin')
    }

    await this.#window.navigate(url.pathname + url.search)
    this.#url = url.pathname + url.search
    return this
  }
}

export class Clients {
  async get (id) {
    state.channel.postMessage({
      clients: {
        get: { id }
      }
    })

    const result = await new Promise((resolve) => {
      const timeout = setTimeout(onTimeout, CLIENT_GET_TIMEOUT)

      state.channel.addEventListener('message', onMessage)

      function onMessage (event) {
        if (event.data?.clients?.get?.result?.client?.id === id) {
          clearTimeout(timeout)
          state.channel.removeEventListener('message', onMessage)
          resolve(event.data.clients.get.result.client)
        }
      }

      function onTimeout () {
        state.channel.removeEventListener('message', onMessage)
        resolve(null)
      }
    })

    if (result) {
      return new Client(result)
    }
  }

  async matchAll (options = null) {
    state.channel.postMessage({
      clients: {
        matchAll: options ?? {}
      }
    })

    const results = await new Promise((resolve) => {
      setTimeout(onTimeout, CLIENT_MATCH_ALL_TIMEOUT)
      const clients = []

      state.channel.addEventListener('message', onMessage)

      function onMessage (event) {
        if (event.data?.clients?.matchAll?.result?.client) {
          const { client } = event.data.client.matchAll.result
          if (!options?.type || options.type === 'all') {
            clients.push(client)
          } else if (options.type === client.type) {
            clients.push(client)
          }
        }
      }

      function onTimeout () {
        state.channel.removeEventListener('message', onMessage)
        resolve(clients)
      }
    })

    return results.map((result) => new Client(result))
  }

  async openWindow (url, options = null) {
    const windows = await application.getWindows()
    const indices = Object.keys(windows)
      .map((key) => parseInt(key))
      .filter((index) => !Number.isNaN(index) && index < MAX_WINDOWS)
      .sort()

    const index = indices.pop() + 1

    if (index < MAX_WINDOWS) {
      throw new DOMException('Max windows are opened', 'InvalidAccessError')
    }

    const window = await application.createWindow({ ...options, index, path: url })

    return new WindowClient({
      frameType: 'top-level',
      type: 'window',
      window,
      url
    })
  }

  async claim () {
    state.channel.postMessage({
      clients: {
        claim: {
          scope: state.serviceWorker.scope,
          scriptURL: state.serviceWorker.scriptURL
        }
      }
    })
  }
}

export default new Clients()
