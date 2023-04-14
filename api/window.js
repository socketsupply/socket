// @ts-check
/**
 * @module Window
 *
 * Provides ApplicationWindow class and methods
 *
 * Usaully you don't need to use this module directly, instance of ApplicationWindow
 * are returned by the methods of the {@link module:Application Application} module.
 */

import * as statuses from './window/constants.js'
import ipc, { primordials } from './ipc.js'
import { isValidPercentageValue } from './util.js'

/**
 * @param {string} url
 * @return {string}
 * @ignore
 */
export function formatFileUrl (url) {
  return `file://${primordials.cwd}/${url}`
}

/**
 * @class ApplicationWindow
 * Represents a window in the application
 */
export class ApplicationWindow {
  #index
  #options
  #senderWindowIndex = globalThis.__args.index
  #listeners = {}
  // TODO(@chicoxyzzy): add parent and children? (needs native process support)

  static constants = statuses

  constructor ({ index, ...options }) {
    this.#index = index
    this.#options = options
  }

  #updateOptions (response) {
    const { data, err } = response
    if (err) {
      throw new Error(err)
    }
    const { index, ...options } = data
    this.#options = options
    return data
  }

  /**
   * Get the index of the window
   * @return {number} - the index of the window
   * @readonly
   */
  get index () {
    return this.#index
  }

  /**
   * Get the size of the window
   * @return {{ width: number, height: number }} - the size of the window
   */
  getSize () {
    return {
      width: this.#options.width,
      height: this.#options.height
    }
  }

  /**
   * Get the title of the window
   * @return {string} - the title of the window
   */
  getTitle () {
    return this.#options.title
  }

  /**
   * Get the status of the window
   * @return {string} - the status of the window
   */
  getStatus () {
    return this.#options.status
  }

  /**
   * Close the window
   * @return {Promise<object>} - the options of the window
   */
  async close () {
    const { data, err } = await ipc.send('window.close', {
      index: this.#senderWindowIndex,
      targetWindowIndex: this.#index
    })
    if (err) {
      throw err
    }
    return data
  }

  /**
   * Shows the window
   * @return {Promise<ipc.Result>}
   */
  async show () {
    const response = await ipc.send('window.show', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateOptions(response)
  }

  /**
   * Hides the window
   * @return {Promise<ipc.Result>}
   */
  async hide () {
    const response = await ipc.send('window.hide', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateOptions(response)
  }

  /**
   * Sets the title of the window
   * @param {string} title - the title of the window
   * @return {Promise<ipc.Result>}
   */
  async setTitle (title) {
    const response = await ipc.send('window.setTitle', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, value: title })
    return this.#updateOptions(response)
  }

  /**
   * Sets the size of the window
   * @param {object} opts - an options object
   * @param {(number|string)=} opts.width - the width of the window
   * @param {(number|string)=} opts.height - the height of the window
   * @return {Promise<ipc.Result>}
   * @throws {Error} - if the width or height is invalid
   */
  async setSize (opts) {
    // default values
    const options = {
      targetWindowIndex: this.#index,
      index: this.#senderWindowIndex
    }

    if ((opts.width != null && typeof opts.width !== 'number' && typeof opts.width !== 'string') ||
      (typeof opts.width === 'string' && !isValidPercentageValue(opts.width)) ||
      (typeof opts.width === 'number' && !(Number.isInteger(opts.width) && opts.width > 0))) {
      throw new Error(`Window width must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.width} instead.`)
    }
    if (typeof opts.width === 'string' && isValidPercentageValue(opts.width)) {
      options.width = opts.width
    }
    if (typeof opts.width === 'number') {
      options.width = opts.width.toString()
    }

    if ((opts.height != null && typeof opts.height !== 'number' && typeof opts.height !== 'string') ||
      (typeof opts.height === 'string' && !isValidPercentageValue(opts.height)) ||
      (typeof opts.height === 'number' && !(Number.isInteger(opts.height) && opts.height > 0))) {
      throw new Error(`Window height must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.height} instead.`)
    }
    if (typeof opts.height === 'string' && isValidPercentageValue(opts.height)) {
      options.height = opts.height
    }
    if (typeof opts.height === 'number') {
      options.height = opts.height.toString()
    }

    const response = await ipc.send('window.setSize', options)
    return this.#updateOptions(response)
  }

  /**
   * Navigate the window to a given path
   * @param {object} path - file path
   * @return {Promise<ipc.Result>}
   */
  async navigate (path) {
    const response = await ipc.send('window.navigate', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, url: formatFileUrl(path) })
    return this.#updateOptions(response)
  }

  /**
   * Opens the Web Inspector for the window
   * @return {Promise<object>}
   */
  async showInspector () {
    const { data, err } = await ipc.send('window.showInspector', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    if (err) {
      throw err
    }
    return data
  }

  /**
   * Sets the background color of the window
   * @param {object} opts - an options object
   * @param {number} opts.red - the red value
   * @param {number} opts.green - the green value
   * @param {number} opts.blue - the blue value
   * @param {number} opts.alpha - the alpha value
   * @return {Promise<object>}
   */
  async setBackgroundColor (opts) {
    const response = await ipc.send('window.setBackgroundColor', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, ...opts })
    return this.#updateOptions(response)
  }

  /**
   * Opens a native context menu.
   * @param {object} options - an options object
   * @return {Promise<object>}
   */
  async setContextMenu (options) {
    const o = Object
      .entries(options)
      .flatMap(a => a.join(':'))
      .join('_')
    const { data, err } = await ipc.send('window.setContextMenu', o)
    if (err) {
      throw err
    }
    return data
  }

  /**
   * Shows a native open file dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showOpenFilePicker (options) {
    console.warn('window.showOpenFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { type: 'open', ...options })
    return typeof data === 'string' ? data.split('\n') : []
  }

  /**
   * Shows a native save file dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showSaveFilePicker (options) {
    console.warn('window.showSaveFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { type: 'save', ...options })
    return typeof data === 'string' ? data.split('\n') : []
  }

  /**
   * Shows a native directory dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showDirectoryFilePicker (options) {
    console.warn('window.showDirectoryFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { allowDirs: true, ...options })
    return typeof data === 'string' ? data.split('\n') : []
  }

  /**
   * Sends an IPC message to the window or to qthe backend.
   * @param {object} options - an options object
   * @param {number=} options.window - the window to send the message to
   * @param {boolean=} [options.backend = false] - whether to send the message to the backend
   * @param {string} options.event - the event to send
   * @param {(string|object)=} options.value - the value to send
   * @returns
   */
  async send (options) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.send can only be used from the current window')
    }
    if (!Number.isInteger(options.window) && options.backend !== true) {
      throw new Error('window should be an integer')
    }
    if (options.backend === true && options.window != null) {
      throw new Error('backend option cannot be used together with window option')
    }
    if (typeof options.event !== 'string' || options.event.length === 0) {
      throw new Error('event should be a non-empty string')
    }

    const value = typeof options.value !== 'string' ? JSON.stringify(options.value) : options.value

    if (options.backend === true) {
      return await ipc.send('process.write', {
        index: this.#senderWindowIndex,
        event: options.event,
        value: value ?? ''
      })
    }

    return await ipc.send('window.send', {
      index: this.#senderWindowIndex,
      targetWindowIndex: options.window,
      event: options.event,
      value: encodeURIComponent(value)
    })
  }

  /**
   * Opens an URL in the default browser.
   * @param {object} options
   * @returns {Promise<ipc.Result>}
   */
  async openExternal (options) {
    return await ipc.send('platform.openExternal', options)
  }

  // public EventEmitter methods
  /**
   * Adds a listener to the window.
   * @param {string} event - the event to listen to
   * @param {function(*): void} cb - the callback to call
   * @returns {void}
   */
  addListener (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.addListener can only be used from the current window')
    }
    if (!(event in this.#listeners)) {
      this.#listeners[event] = []
    }
    this.#listeners[event].push(cb)
    globalThis.addEventListener(event, cb)
  }

  /**
   * Adds a listener to the window. An alias for `addListener`.
   * @param {string} event - the event to listen to
   * @param {function(*): void} cb - the callback to call
   * @returns {void}
   * @see addListener
   */
  on (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.on can only be used from the current window')
    }
    if (!(event in this.#listeners)) {
      this.#listeners[event] = []
    }
    this.#listeners[event].push(cb)
    globalThis.addEventListener(event, cb)
  }

  /**
   * Adds a listener to the window. The listener is removed after the first call.
   * @param {string} event - the event to listen to
   * @param {function(*): void} cb - the callback to call
   * @returns {void}
   */
  once (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.once can only be used from the current window')
    }
    if (!(event in this.#listeners)) {
      this.#listeners[event] = []
    }
    globalThis.addEventListener(event, cb, { once: true })
  }

  /**
   * Removes a listener from the window.
   * @param {string} event - the event to remove the listener from
   * @param {function(*): void} cb - the callback to remove
   * @returns {void}
   */
  removeListener (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.removeListener can only be used from the current window')
    }
    this.#listeners[event] = this.#listeners[event].filter(listener => listener !== cb)
    globalThis.removeEventListener(event, cb)
  }

  /**
   * Removes all listeners from the window.
   * @param {string} event - the event to remove the listeners from
   * @returns {void}
   */
  removeAllListeners (event) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.removeAllListeners can only be used from the current window')
    }
    for (const cb of this.#listeners[event]) {
      globalThis.removeEventListener(event, cb)
    }
  }

  /**
   * Removes a listener from the window. An alias for `removeListener`.
   * @param {string} event - the event to remove the listener from
   * @param {function(*): void} cb - the callback to remove
   * @returns {void}
   * @see removeListener
   */
  off (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.off can only be used from the current window')
    }
    this.#listeners[event] = this.#listeners[event].filter(listener => listener !== cb)
    globalThis.removeEventListener(event, cb)
  }
}

export default ApplicationWindow
export const constants = statuses
