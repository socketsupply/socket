// @ts-check
/**
 * @module Window
 *
 * Provides Window class and methods
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

// TODO(@chicoxyzzy): JSDoc
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

  get index () {
    return this.#index
  }

  getSize () {
    return {
      width: this.#options.width,
      height: this.#options.height
    }
  }

  getTitle () {
    return this.#options.title
  }

  getStatus () {
    return this.#options.status
  }

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
   * @param {object} path - file path
   * @return {Promise<ipc.Result>}
   */
  async navigate (path) {
    const response = await ipc.send('window.navigate', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, url: formatFileUrl(path) })
    return this.#updateOptions(response)
  }

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
   * @return {Promise<ipc.Result>}
   */
  async setBackgroundColor (opts) {
    const response = await ipc.send('window.setBackgroundColor', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, ...opts })
    return this.#updateOptions(response)
  }

  /**
   * Opens a native context menu.
   * @param {object} options - an options object
   * @return {Promise}
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

  // TODO(@heapwolf) the properties do not yet conform to the MDN spec
  async showOpenFilePicker (o) {
    console.warn('window.showOpenFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { type: 'open', ...o })
    return typeof data === 'string' ? data.split('\n') : []
  }

  // TODO(@heapwolf) the properties do not yet conform to the MDN spec
  async showSaveFilePicker (o) {
    console.warn('window.showSaveFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { type: 'save', ...o })
    return typeof data === 'string' ? data.split('\n') : []
  }

  // TODO(@heapwolf) the properties do not yet conform to the MDN spec
  async showDirectoryFilePicker (o) {
    console.warn('window.showDirectoryFilePicker may not conform to the standard')
    const { data } = await ipc.send('dialog', { allowDirs: true, ...o })
    return typeof data === 'string' ? data.split('\n') : []
  }

  /**
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
    if (typeof options.value !== 'string') {
      options.value = JSON.stringify(options.value)
    }

    const value = typeof options.value === 'string' ? options.value : JSON.stringify(options.value)

    if (options.backend === true) {
      return await ipc.send('process.write', {
        ...options,
        index: this.#senderWindowIndex
      })
      // return await ipc.send('process.write', {
      //   index: this.#senderWindowIndex,
      //   event: options.event,
      //   value: options.value
      // })
    }

    return await ipc.send('window.send', {
      index: this.#senderWindowIndex,
      targetWindowIndex: options.window,
      event: options.event,
      value: encodeURIComponent(value)
    })
  }

  /**
   *
   * @param {object} options
   * @returns {Promise<ipc.Result>}
   */
  async openExternal (options) {
    return await ipc.send('platform.openExternal', options)
  }

  // public EventEmitter methods
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
  once (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.once can only be used from the current window')
    }
    if (!(event in this.#listeners)) {
      this.#listeners[event] = []
    }
    globalThis.addEventListener(event, cb, { once: true })
  }
  removeListener (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.removeListener can only be used from the current window')
    }
    this.#listeners[event] = this.#listeners[event].filter(listener => listener !== cb)
    globalThis.removeEventListener(event, cb)
  }
  removeAllListeners (event) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.removeAllListeners can only be used from the current window')
    }
    for (const cb of this.#listeners[event]) {
      globalThis.removeEventListener(event, cb)
    }
  }
  off (event, cb) {
    if (this.#index !== this.#senderWindowIndex) {
      throw new Error('window.off can only be used from the current window')
    }
    this.#listeners[event] = this.#listeners[event].filter(listener => listener !== cb)
    globalThis.removeEventListener(event, cb)
  }

  // TODO(@chicoxyzzy): implement on method
}

export default ApplicationWindow
export const constants = statuses
