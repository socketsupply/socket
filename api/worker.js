import console from './console.js'

// eslint-disable-next-line no-new-func
const GlobalWorker = new Function('return Worker')()

class UnsupportedWorker extends EventTarget {
  constructor () {
    super()
    console.warn('Worker is not supported in this environment')
  }
}

/**
 * @type {import('dom').Worker}
 */
export const Worker = GlobalWorker || UnsupportedWorker

export default Worker
