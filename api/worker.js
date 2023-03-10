import console from './console.js'

export const Worker = globalThis.Worker || class extends EventTarget {
  constructor () {
    super()
    console.warn('Worker is not supported in this environment')
  }
}

export default Worker
