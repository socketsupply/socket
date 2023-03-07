import { CustomEvent } from './events.js'

/**
 * An interface for registering callbacks for various hooks in
 * the runtime.
 * @ignore
 */
// eslint-disable-next-line new-parens
export default new class Hooks extends EventTarget {
  /**
   * @ignore
   */
  constructor () {
    super()

    if (this.isGlobalReady) {
      queueMicrotask(() => {
        this.dispatchEvent(new CustomEvent('ready'))
      })
    } else if (this.document) {
      this.document?.addEventListener('domcontentloaded', () => {
        this.document.addEventListener('readystatechange', () => {
          if (this.document.readyState === 'complete') {
            this.global.addEventListener('load', () => {
              queueMicrotask(() => this.dispatchEvent(new CustomEvent('ready')))
            })
          }
        })
      })
    }

    this.global.addEventListener('error', (err) => {
      this.dispatchEvent(new CustomEvent('error', { detail: err }))
    })

    this.global.addEventListener('messageerror', (err) => {
      this.dispatchEvent(new CustomEvent('error', { detail: err }))
    })

    this.global.addEventListener('unhandledrejection', (err) => {
      this.dispatchEvent(new CustomEvent('error', { detail: err }))
    })
  }

  /**
   * Reference to global object
   * @type {Global}
   */
  get global () {
    return globalThis || new EventTarget()
  }

  /**
   * Returns document for global
   * @type {Document}
   */
  get document () {
    return this.global.document ?? null
  }

  /**
   * Predicate for determining if globalis ready.
   * @type {boolean}
   */
  get isGlobalReady () {
    return this.document?.readyState === 'complete'
  }

  /**
   * Wait for the global Window and Document to be ready. The callback
   * function is called exactly once.
   * @param {function} callback
   * @return {function}
   */
  onReady (callback) {
    if (this.isGlobalReady) {
      queueMicrotask(callback)
    } else {
      this.addEventListener('ready', callback, { once: true })
    }

    return () => this.removeEventListener('ready', callback)
  }

  /**
   * Calls callback when a global exception occurs.
   * @param {function} callback
   * @return {function}
   */
  onError (callback) {
    this.addEventListener('error', callback)
    return () => this.removeEventListener('error', callback)
  }

  /**
   * Subscribes to the global data pipe calling callback when
   * new data is emitted on the global Window.
   * @param {function} callback
   * @return {function}
   */
  onData (callback) {
    this.addEventListener('data', callback)
    return () => this.removeEventListener('data', callback)
  }
}
