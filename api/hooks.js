import { Event, CustomEvent, ErrorEvent, MessageEvent } from './events.js'

/**
 * An interface for registering callbacks for various hooks in
 * the runtime.
 * @ignore
 */
// eslint-disable-next-line new-parens
export default new class Hooks extends EventTarget {
  didGlobalLoad = false
  didRuntimeInit = false

  /**
   * @ignore
   */
  constructor () {
    super()

    if (!this.document && !this.global.window) { // worker
      queueMicrotask(() => {
        this.didGlobalLoad = true
        queueMicrotask(() => this.dispatchEvent(new CustomEvent('load')))
        queueMicrotask(() => this.dispatchEvent(new CustomEvent('ready')))
      })
    }

    if (this.document?.readyState === 'complete') {
      this.didGlobalLoad = true
    }

    this.global.addEventListener('load', () => {
      this.didGlobalLoad = true
    })

    this.global.addEventListener('init', () => {
      this.didRuntimeInit = true
      if (this.didGlobalLoad) {
        queueMicrotask(() => this.dispatchEvent(new CustomEvent('ready')))
      }
    })

    const events = [
      'data',
      'init',
      'load',
      'message',
      'online',
      'offline',
      'error',
      'messageerror',
      'unhandledrejection'
    ]

    for (const type of events) {
      this.global.addEventListener(type, (event) => {
        const { type, data, detail = null, error } = event
        if (error) {
          this.dispatchEvent(new ErrorEvent(type, { error, detail }))
        } else if (type && data) {
          this.dispatchEvent(new MessageEvent(type, { data, detail }))
        } else if (detail) {
          this.dispatchEvent(new CustomEvent(type, { detail }))
        } else {
          this.dispatchEvent(new Event(type))
        }
      })
    }

    if (this.isDocumentReady && this.isRuntimeReady) {
      // artificially notify
      queueMicrotask(() => {
        this.dispatchEvent(new CustomEvent('load'))
        this.didGlobalLoad = true
        queueMicrotask(() => {
          this.didRuntimeInit = true
          this.dispatchEvent(new CustomEvent('init'))
          queueMicrotask(() => {
            this.dispatchEvent(new CustomEvent('ready'))
          })
        })
      })
    } else if (this.document) {
      this.document?.addEventListener('DOMContentLoaded', () => {
        this.document.addEventListener('readystatechange', () => {
          if (this.document.readyState === 'complete') {
            if (this.didGlobalLoad) {
              console.log(this.didRuntimeInit)
              this.global.addEventListener('init', () => {
                queueMicrotask(() => {
                  this.dispatchEvent(new CustomEvent('ready'))
                })
              }, { once: true })
            } else {
              this.global.addEventListener('load', () => {
                this.global.addEventListener('init', () => {
                  queueMicrotask(() => {
                    this.dispatchEvent(new CustomEvent('ready'))
                  })
                }, { once: true })
              }, { once: true })
            }
          }
        })
      }, { once: true })
    }
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
   * Predicate for determining if the global document is ready.
   * @type {boolean}
   */
  get isDocumentReady () {
    return !this.document && !this.global.window && this.global.self
      ? true
      : this.document?.readyState === 'complete'
  }

  /**
   * Predicate for determining if the global object is ready.
   * @type {boolean}
   */
  get isGlobalReady () {
    return this.isDocumentReady && this.didGlobalLoad
  }

  /**
   * Predicate for determining if the runtime is ready.
   * @type {boolean}
   */
  get isRuntimeReady () {
    return this.didRuntimeInit
  }

  /**
   * Predicate for determining if everything is ready.
   * @type {boolean}
   */
  get isReady () {
    return this.isDocumentReady && this.isGlobalReady && this.isRuntimeReady
  }

  /**
   * Predicate for determining if the runtime is working online.
   * @type {boolean}
   */
  get isOnline () {
    return this.global.navigator?.onLine || false
  }

  /**
   * Wait for the global Window, Document, and Runtime to be ready.
   * The callback function is called exactly once.
   * @param {function} callback
   * @return {function}
   */
  onReady (callback) {
    if (this.isDocumentReady && this.isGlobalReady && this.isRuntimeReady) {
      setTimeout(callback)
      return () => undefined
    } else {
      this.addEventListener('ready', callback, { once: true })
      return () => this.removeEventListener('ready', callback)
    }
  }

  /**
   * Wait for the global Window and Document to be ready. The callback
   * function is called exactly once.
   * @param {function} callback
   * @return {function}
   */
  onLoad (callback) {
    if (this.isGlobalReady) {
      setTimeout(callback)
      return () => undefined
    } else {
      this.addEventListener('load', callback, { once: true })
      return () => this.removeEventListener('load', callback)
    }
  }

  /**
   * Wait for the runtime to be ready. The callback
   * function is called exactly once.
   * @param {function} callback
   * @return {function}
   */
  onInit (callback) {
    if (this.isRuntimeReady) {
      setTimeout(callback)
      return () => undefined
    } else {
      this.addEventListener('init', callback, { once: true })
      return () => this.removeEventListener('init', callback)
    }
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

  /**
   * Subscribes to global messages likely from an external `postMessage`
   * invocation.
   * @param {function} callback
   * @return {function}
   */
  onMessage (callback) {
    this.addEventListener('message', callback)
    return () => this.removeEventListener('message', callback)
  }

  /**
   * Calls callback when runtime is working online.
   * @param {function} callback
   * @return {function}
   */
  onOnline (callback) {
    this.addEventListener('online', callback)
    return () => this.removeEventListener('online', callback)
  }

  /**
   * Calls callback when runtime is not working online.
   * @param {function} callback
   * @return {function}
   */
  onOffline (callback) {
    this.addEventListener('offline', callback)
    return () => this.removeEventListener('offline', callback)
  }
}
