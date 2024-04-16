import application from '../application.js'
import debug from './debug.js'
import ipc from '../ipc.js'

export const channel = new BroadcastChannel('socket.runtime.serviceWorker.state')

const descriptors = {
  channel: {
    configurable: false,
    enumerable: false,
    value: channel
  },

  clients: {
    configurable: false,
    enumerable: true,
    value: Object.create(null)
  },

  notify: {
    configurable: false,
    enumerable: false,
    writable: false,
    async value (type) {
      channel.postMessage({ [type]: this[type] })

      if (this.id && type === 'serviceWorker') {
        debug(
          '[%s]: ServiceWorker (%s) updated state to "%s"',
          new URL(this.serviceWorker.scriptURL).pathname.replace(/^\/socket\//, 'socket:'),
          this.id,
          this.serviceWorker.state
        )

        await ipc.request('serviceWorker.updateState', {
          id: this.id,
          scope: this.serviceWorker.scope,
          state: this.serviceWorker.state,
          scriptURL: this.serviceWorker.scriptURL,
          workerURL: globalThis.location.href
        })
      }
    }
  },

  serviceWorker: {
    configurable: false,
    enumerable: true,
    value: Object.create(null, {
      scope: {
        configurable: false,
        enumerable: true,
        writable: true,
        value: globalThis.window
          ? new URL('.', globalThis.location.href).pathname
          : '/'
      },

      scriptURL: {
        configurable: false,
        enumerable: false,
        writable: true,
        value: null
      },

      state: {
        configurable: false,
        enumerable: true,
        writable: true,
        value: 'parsed'
      },

      id: {
        configurable: false,
        enumerable: true,
        writable: true,
        value: null
      }
    })
  },

  id: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: null
  },

  fetch: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: null
  },

  install: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: null
  },

  activate: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: null
  },

  reportError: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: globalThis.reportError.bind(globalThis)
  }
}

export const state = Object.create(null, descriptors)

channel.addEventListener('message', (event) => {
  if (event.data?.serviceWorker) {
    let href = globalThis.location.href
    if (href.startsWith('blob:')) {
      href = new URL(href).pathname
    }

    const scope = new URL('.', href).pathname
    if (scope.startsWith(event.data.serviceWorker.scope)) {
      Object.assign(state.serviceWorker, event.data.serviceWorker)
    }
  } else if (event.data?.clients?.get?.id) {
    if (event.data.clients.get.id === globalThis.__args.client.id) {
      channel.postMessage({
        clients: {
          get: {
            result: {
              client: {
                id: globalThis.__args.client.id,
                url: globalThis.location.pathname + globalThis.location.search,
                type: globalThis.__args.client.type,
                index: globalThis.__args.index,
                frameType: globalThis.__args.client.frameType
              }
            }
          }
        }
      })
    }
  } else if (event.data?.clients?.matchAll) {
    const type = event.data.clients.matchAll?.type ?? 'window'
    if (type === 'all' || type === globalThis.__args.type) {
      channel.postMessage({
        clients: {
          matchAll: {
            result: {
              client: {
                id: globalThis.__args.client.id,
                url: globalThis.location.pathname + globalThis.location.search,
                type: globalThis.__args.client.type,
                index: globalThis.__args.index,
                frameType: globalThis.__args.client.frameType
              }
            }
          }
        }
      })
    }
  }
})

if (globalThis.document) {
  channel.addEventListener('message', async (event) => {
    if (event.data?.client?.id === globalThis.__args.client.id) {
      if (event.data.client.focus === true) {
        const currentWindow = await application.getCurrentWindow()
        try {
          await currentWindow.restore()
        } catch {}
        globalThis.focus()
      }
    }
  })

  globalThis.document.addEventListener('visibilitychange', (event) => {
    channel.postMessage({
      client: {
        id: globalThis.__args.client.id,
        visibilityState: globalThis.document.visibilityState
      }
    })
  })

  globalThis.addEventListener('focus', (event) => {
    channel.postMessage({
      client: {
        id: globalThis.__args.client.id,
        focused: true
      }
    })
  })

  globalThis.addEventListener('blur', (event) => {
    channel.postMessage({
      client: {
        id: globalThis.__args.client.id,
        focused: false
      }
    })
  })
}

export default state
