/* global reportError */
import ipc from '../ipc.js'

export const channel = new BroadcastChannel('service-worker-state')

const descriptors = {
  channel: {
    configurable: false,
    enumerable: false,
    value: channel
  },

  notify: {
    configurable: false,
    enumerable: false,
    writable: false,
    async value (type) {
      channel.postMessage({ [type]: this[type] })

      if (type === 'serviceWorker') {
        await ipc.request('serviceWorker.updateState', {
          id: this.id,
          state: this.serviceWorker.state
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
        value: '/'
      },

      state: {
        configurable: false,
        enumerable: true,
        writable: true,
        value: 'parsed'
      }
    })
  }
}

if (!globalThis.window) {
  Object.assign(descriptors, {
    id: {
      configurable: false,
      enumerable: false,
      writable: true,
      value: null
    },

    scriptURL: {
      configurable: false,
      enumerable: false,
      writable: true,
      value: null
    },

    scope: {
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
      value: reportError
    }
  })
}

export const state = Object.create(null, descriptors)

channel.addEventListener('message', (event) => {
  if (event.data.serviceWorker) {
    const scope = new URL(globalThis.location.href).pathname
    if (scope.startsWith(event.data.serviceWorker.scope)) {
      Object.assign(state.serviceWorker, event.data.serviceWorker)
    }
  }
})

export default state
