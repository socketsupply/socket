import SharedWorker from './internal/shared-worker.js'
import console from './console.js'

export { SharedWorker }

// eslint-disable-next-line no-new-func
const GlobalWorker = new Function('return this.Worker')()

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

/**
 * @type {import('dom').SharedWorker}
 */
export default Worker
