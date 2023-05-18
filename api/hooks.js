/**
 * @module hooks
 *
 * An interface for registering callbacks for various hooks in
 * the runtime.
 *
 * Example usage:
 * ```js
 * import hooks from 'socket:hooks'
 *
 * hooks.onLoad(() => {
 *   // called when the runtime context has loaded,
 *   // but not ready or initialized
 * })
 *
 * hooks.onReady(() => {
 *   // called when the runtime is ready, but not initialized
 * })
 *
 * hooks.onInit(() => {
 *   // called when the runtime is initialized
 * })
 *
 * hooks.onError((event) => {
 *   // called when 'error', 'messageerror', and 'unhandledrejection' error
 *   // events are dispatched on the global object
 * })
 *
 * hooks.onData((event) => {
 *   // called when 'data' events are dispatched on the global object
 * })
 *
 * hooks.onMessage((event) => {
 *   // called when 'message' events are dispatched on the global object
 * })
 *
 * hooks.onOnline((event) => {
 *   // called when 'online' events are dispatched on the global object
 * })
 *
 * hooks.onOffline((event) => {
 *   // called when 'offline' events are dispatched on the global object
 * })
 * ```
 */
import { Event, CustomEvent, ErrorEvent, MessageEvent } from './events.js'

function addEventListenerOnce (target, type, callback) {
  target.addEventListener(type, callback, { once: true })
}

async function waitForEvent (target, type) {
  return await new Promise((resolve) => {
    addEventListenerOnce(target, type, resolve)
  })
}

function dispatchEvent (target, event) {
  queueMicrotask(() => target.dispatchEvent(event))
}

function dispatchInitEvent (target) {
  dispatchEvent(target, new InitEvent())
}

function dispatchLoadEvent (target) {
  dispatchEvent(target, new LoadEvent())
}

function dispatchReadyEvent (target) {
  dispatchEvent(target, new ReadyEvent())
}

function proxyGlobalEvents (global, target) {
  const GLOBAL_EVENTS = [
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

  for (const type of GLOBAL_EVENTS) {
    global.addEventListener(type, (event) => {
      const { type, data, detail = null, error } = event
      if (error) {
        target.dispatchEvent(new ErrorEvent(type, { error, detail }))
      } else if (type && data) {
        target.dispatchEvent(new MessageEvent(type, { data, detail }))
      } else if (detail) {
        target.dispatchEvent(new CustomEvent(type, { detail }))
      } else {
        target.dispatchEvent(new Event(type))
      }
    })
  }
}

const RUNTIME_INIT_EVENT_NAME = '__runtime_init__'

// state
let isGlobalLoaded = false
let isRuntimeInitialized = false

/**
 * An event dispatched when the runtime has been initialized.
 */
export class InitEvent extends Event {
  constructor () { super('init') }
}

/**
 * An event dispatched when the runtime global has been loaded.
 */
export class LoadEvent extends Event {
  constructor () { super('load') }
}

/**
 * An event dispatched when the runtime is considered ready.
 */
export class ReadyEvent extends Event {
  constructor () { super('ready') }
}

/**
 * An interface for registering callbacks for various hooks in
 * the runtime.
 */
// eslint-disable-next-line new-parens
export class Hooks extends EventTarget {
  /**
   * @ignore
   */
  constructor () {
    super()
    this.#init()
  }

  /**
   * Reference to global object
   * @type {Global}
   */
  get global () {
    return globalThis || new EventTarget()
  }

  /**
   * Returns `document` in global.
   * @type {Document}
   */
  get document () {
    return this.global.document ?? null
  }

  /**
   * Returns `document` in global.
   * @type {Window}
   */
  get window () {
    return this.global.window ?? null
  }

  /**
   * Predicate for determining if the global document is ready.
   * @type {boolean}
   */
  get isDocumentReady () {
    return this.document?.readyState === 'complete'
  }

  /**
   * Predicate for determining if the global object is ready.
   * @type {boolean}
   */
  get isGlobalReady () {
    return isGlobalLoaded
  }

  /**
   * Predicate for determining if the runtime is ready.
   * @type {boolean}
   */
  get isRuntimeReady () {
    return isRuntimeInitialized
  }

  /**
   * Predicate for determining if everything is ready.
   * @type {boolean}
   */
  get isReady () {
    return this.isRuntimeReady && (this.isWorkerContext || this.isDocumentReady)
  }

  /**
   * Predicate for determining if the runtime is working online.
   * @type {boolean}
   */
  get isOnline () {
    return this.global.navigator?.onLine || false
  }

  /**
   * Predicate for determining if the runtime is in a Worker context.
   * @type {boolean}
   */
  get isWorkerContext () {
    return Boolean(!this.document && !this.global.window && this.global.self)
  }

  /**
   * Predicate for determining if the runtime is in a Window context.
   * @type {boolean}
   */
  get isWindowContext () {
    return Boolean(this.document && this.global.window)
  }

  /**
   * Internal hooks initialization.
   * Event order:
   *  1. 'DOMContentLoaded'
   *  2. 'load'
   *  3. 'init'
   * @ignore
   * @private
   */
  async #init () {
    const { isWorkerContext, document, global } = this
    const readyState = document?.readyState

    isRuntimeInitialized = Boolean(global.__RUNTIME_INIT_NOW__)

    proxyGlobalEvents(global, this)

    // if runtime is initialized, then 'DOMContentLoaded' (document),
    // 'load' (window), and the 'init' (window) events have all been dispatched
    // prior to hook initialization
    if (isRuntimeInitialized) {
      dispatchLoadEvent(this)
      dispatchReadyEvent(this)
      dispatchInitEvent(this)
      return
    }

    addEventListenerOnce(global, RUNTIME_INIT_EVENT_NAME, () => {
      isRuntimeInitialized = true
      dispatchInitEvent(this)
    })

    if (!isWorkerContext && readyState !== 'complete') {
      await waitForEvent(global, 'load')
    }

    isGlobalLoaded = true
    dispatchLoadEvent(this)
    dispatchReadyEvent(this)
  }

  /**
   * Wait for the global Window, Document, and Runtime to be ready.
   * The callback function is called exactly once.
   * @param {function} callback
   * @return {function}
   */
  onReady (callback) {
    if (this.isReady) {
      const timeout = setTimeout(callback)
      return () => clearTimeout(timeout)
    } else {
      addEventListenerOnce(this, 'ready', callback)
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
      const timeout = setTimeout(callback)
      return () => clearTimeout(timeout)
    } else {
      addEventListenerOnce(this, 'load', callback)
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
      const timeout = setTimeout(callback)
      return () => clearTimeout(timeout)
    } else {
      addEventListenerOnce(this, 'init', callback)
      return () => this.removeEventListener('init', callback)
    }
  }

  /**
   * Calls callback when a global exception occurs.
   * 'error', 'messageerror', and 'unhandledrejection' events are handled here.
   * @param {function} callback
   * @return {function}
   */
  onError (callback) {
    this.addEventListener('error', callback)
    this.addEventListener('messageerror', callback)
    this.addEventListener('unhandledrejection', callback)
    return () => {
      this.removeEventListener('error', callback)
      this.removeEventListener('messageerror', callback)
      this.removeEventListener('unhandledrejection', callback)
    }
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

export default new Hooks()
