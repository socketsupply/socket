const descriptors = {
  channel: {
    configurable: false,
    enumerable: false,
    value: null
  },

  sharedWorker: {
    configurable: false,
    enumerable: true,
    value: Object.create(null, {
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

  connect: {
    configurable: false,
    enumerable: false,
    writable: true,
    value: null
  },

  env: {
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

export default state
