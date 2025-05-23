/* global CloseEvent */
// @ts-check
/**
 * @module window
 *
 * Provides ApplicationWindow class and methods
 *
 * Don't use this module directly, get instances of ApplicationWindow with
 * `socket:application` methods like `getCurrentWindow`, `createWindow`,
 * `getWindow`, and `getWindows`.
 */

import { isValidPercentageValue } from './util.js'
import * as statuses from './window/constants.js'
import location from './location.js'
import { URL } from './url.js'
import client from './application/client.js'
import hotkey from './window/hotkey.js'
import menu from './application/menu.js'
import ipc from './ipc.js'

/**
 * @param {string} url
 * @return {string}
 * @ignore
 */
export function formatURL (url) {
  const href = location?.href ?? 'socket:///'
  return String(new URL(url, href))
}

/**
 * @class ApplicationWindow
 * Represents a window in the application
 */
export class ApplicationWindow extends EventTarget {
  #id = null
  #index
  #state
  #channel = null
  #senderWindowIndex = globalThis.__args.index
  #listeners = {}
  // TODO(@chicoxyzzy): add parent and children? (needs native process support)

  static constants = statuses
  static hotkey = hotkey

  constructor ({ index, ...state }) {
    super()
    this.#id = state?.id
    this.#index = index
    this.#state = state
    this.#channel = new BroadcastChannel(`socket.runtime.window.${this.#index}`)

    const window = this

    this.#channel.addEventListener('message', onMessage)
    globalThis.addEventListener('windowclosed', onWindowClosed)
    globalThis.addEventListener('windowhidden', onWindowHidden)

    function onMessage (e) {
      window.dispatchEvent(new MessageEvent('message', e))
    }

    function onWindowClosed (e) {
      // @ts-ignore
      if (e.detail?.index === window.#index) {
        window.dispatchEvent(new CloseEvent('close'))
        window.channel.removeEventListener('message', onMessage)
        globalThis.removeEventListener('windowclosed', onWindowClosed)
        globalThis.removeEventListener('windowhidden', onWindowHidden)
      }
    }

    function onWindowHidden (e) {
      // @ts-ignore
      if (e.detail?.index === window.#index) {
        window.dispatchEvent(new CloseEvent('hide'))
      }
    }
  }

  #updateState (response) {
    const { data, err } = response
    if (err) {
      throw new Error(err)
    }
    const { id, index, ...state } = data
    this.#id = id ?? null
    this.#state = state
    return data
  }

  /**
   * The unique ID of this window.
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * Get the index of the window
   * @return {number} - the index of the window
   */
  get index () {
    return this.#index
  }

  /**
   * @type {import('./window/hotkey.js').default}
   */
  get hotkey () {
    return hotkey
  }

  get state () {
    return this.#state
  }

  /**
   * The broadcast channel for this window.
   * @type {BroadcastChannel}
   */
  get channel () {
    return this.#channel
  }

  /**
   * Get the size of the window
   * @type {{ width: number, height: number }} - the size of the window
   */
  get size () {
    return {
      width: this.#state.width,
      height: this.#state.height
    }
  }

  get location () {
    return this.#state.location ?? null
  }

  /**
   * get  the position of the window
   * @type {{ x: number, y: number }} - the position of the window
   */
  get position () {
    return {
      x: this.#state.position.x,
      y: this.#state.position.y
    }
  }

  /**
   * get  the title of the window
   * @type {string}
   */
  get title () {
    return this.#state.title
  }

  /**
   * @type {string}
   */
  get token () {
    return this.#state.token
  }

  /**
   * get  the status of the window
   * @type {string} - the status of the window
   */
  get status () {
    return this.#state.status
  }

  /**
   * Get the size of the window
   * @return {{ width: number, height: number }} - the size of the window
   */
  getSize () {
    return {
      width: this.#state.width,
      height: this.#state.height
    }
  }

  /**
   * Get the position of the window
   * @return {{ x: number, y: number }} - the position of the window
   */
  getPosition () {
    return {
      x: this.#state.x,
      y: this.#state.y
    }
  }

  /**
   * Get the title of the window
   * @return {string} - the title of the window
   */
  getTitle () {
    return this.#state.title
  }

  /**
   * Get the status of the window
   * @return {string} - the status of the window
   */
  getStatus () {
    return this.#state.status
  }

  /**
   * Close the window
   * @return {Promise<object>} - the options of the window
   */
  async close () {
    const { data, err } = await ipc.request('window.close', {
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
    const response = await ipc.request('window.show', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateState(response)
  }

  /**
   * Hides the window
   * @return {Promise<ipc.Result>}
   */
  async hide () {
    const response = await ipc.request('window.hide', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateState(response)
  }

  /**
   * Maximize the window
   * @return {Promise<ipc.Result>}
   */
  async maximize () {
    const response = await ipc.request('window.maximize', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateState(response)
  }

  /**
   * Minimize the window
   * @return {Promise<ipc.Result>}
   */
  async minimize () {
    const response = await ipc.request('window.minimize', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateState(response)
  }

  /**
   * Restore the window
   * @return {Promise<ipc.Result>}
   */
  async restore () {
    const response = await ipc.request('window.restore', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
    return this.#updateState(response)
  }

  /**
   * Sets the title of the window
   * @param {string} title - the title of the window
   * @return {Promise<ipc.Result>}
   */
  async setTitle (title) {
    const response = await ipc.request('window.setTitle', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, value: title })
    return this.#updateState(response)
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

    const response = await ipc.request('window.setSize', options)
    return this.#updateState(response)
  }

  /**
   * Sets the position of the window
   * @param {object} opts - an options object
   * @param {(number|string)=} opts.x - the x position of the window
   * @param {(number|string)=} opts.y - the y position of the window
   * @return {Promise<object>}
   * @throws {Error} - if the x or y is invalid
   */
  async setPosition (opts) {
    // default values
    const options = {
      targetWindowIndex: this.#index,
      index: this.#senderWindowIndex
    }

    if ((opts.x != null && typeof opts.x !== 'number' && typeof opts.x !== 'string') ||
      (typeof opts.x === 'string' && !isValidPercentageValue(opts.x)) ||
      (typeof opts.x === 'number' && !(Number.isInteger(opts.x) && opts.x > 0))) {
      throw new Error(`Window x must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.x} instead.`)
    }

    if (typeof opts.x === 'string' && isValidPercentageValue(opts.x)) {
      options.x = opts.x
    }

    if (typeof opts.x === 'number') {
      options.x = opts.x.toString()
    }

    if ((opts.y != null && typeof opts.y !== 'number' && typeof opts.y !== 'string') ||
      (typeof opts.y === 'string' && !isValidPercentageValue(opts.y)) ||
      (typeof opts.y === 'number' && !(Number.isInteger(opts.y) && opts.y > 0))) {
      throw new Error(`Window y must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.y} instead.`)
    }

    if (typeof opts.y === 'string' && isValidPercentageValue(opts.y)) {
      options.y = opts.y
    }

    if (typeof opts.y === 'number') {
      options.y = opts.y.toString()
    }

    const response = await ipc.request('window.setPosition', options)
    return this.#updateState(response)
  }

  /**
   * Navigate the window to a given path
   * @param {object} path - file path
   * @return {Promise<ipc.Result>}
   */
  async navigate (path) {
    const response = await ipc.request('window.navigate', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, url: formatURL(path) })
    return this.#updateState(response)
  }

  /**
   * Opens the Web Inspector for the window
   * @return {Promise<object>}
   */
  async showInspector () {
    const { data, err } = await ipc.request('window.showInspector', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
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
    const response = await ipc.request('window.setBackgroundColor', { index: this.#senderWindowIndex, targetWindowIndex: this.#index, ...opts })
    return this.#updateState(response)
  }

  /**
   * Gets the background color of the window
   * @return {Promise<object>}
   */
  async getBackgroundColor () {
    return await ipc.request('window.getBackgroundColor', { index: this.#senderWindowIndex, targetWindowIndex: this.#index })
  }

  /**
   * Opens a native context menu.
   * @param {object} options - an options object
   * @return {Promise<object>}
   */
  async setContextMenu (options) {
    return await menu.context.set(options)
  }

  /**
   * Shows a native open file dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showOpenFilePicker (options) {
    const result = await ipc.request('window.showFileSystemPicker', {
      type: 'open',
      ...options
    })

    if (result.err) {
      throw result.err
    }

    return result.data.paths
  }

  /**
   * Shows a native save file dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showSaveFilePicker (options) {
    const result = await ipc.request('window.showFileSystemPicker', {
      type: 'save',
      ...options
    })

    if (result.err) {
      throw result.err
    }

    return result.data.paths[0] ?? null
  }

  /**
   * Shows a native directory dialog.
   * @param {object} options - an options object
   * @return {Promise<string[]>} - an array of file paths
   */
  async showDirectoryFilePicker (options) {
    const result = await ipc.request('window.showFileSystemPicker', {
      type: 'open',
      allowDirs: true,
      ...options
    })

    if (result.err) {
      throw result.err
    }

    return result.data.paths
  }

  /**
   * This is a high-level API that you should use instead of `ipc.request` when
   * you want to send a message to another window or to the backend.
   *
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
      return await ipc.request('process.write', {
        index: this.#senderWindowIndex,
        event: options.event,
        value: value !== undefined ? JSON.stringify(value) : null
      })
    }

    return await ipc.request('window.send', {
      index: this.#senderWindowIndex,
      targetWindowIndex: options.window,
      event: options.event,
      value: encodeURIComponent(value)
    })
  }

  /**
   * Post a message to a window
   * TODO(@jwerle): research using `BroadcastChannel` instead
   * @param {object} data
   * @return {Promise}
   */
  async postMessage (data) {
    if (this.#index === this.#senderWindowIndex) {
      globalThis.dispatchEvent(new MessageEvent('message', { data }))
    } else {
      return await ipc.request('window.send', {
        index: this.#senderWindowIndex,
        targetWindowIndex: this.#index,
        event: 'message',
        value: encodeURIComponent(data)
      })
    }
  }

  /**
   * Opens an URL in the default application associated with the URL protocol,
   * such as 'https:' for the default web browser.
   * @param {string} value
   * @returns {Promise<{ url: string }>}
   */
  async openExternal (value) {
    const result = await ipc.request('platform.openExternal', value)

    if (result.err) {
      throw result.err
    }

    return result.data
  }

  /**
   * Opens a file in the default file explorer.
   * @param {string} value
   * @returns {Promise}
   */
  async revealFile (value) {
    const result = await ipc.request('platform.revealFile', value)

    if (result.err) {
      throw result.err
    }
  }

  /**
   * Updates wnidow state
   * @return {Promise<ipc.Result>}
   */
  async update () {
    const response = await ipc.request('window', { index: this.#index })
    return this.#updateState(response)
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

export { client, hotkey }

export default ApplicationWindow

/**
 * @ignore
 */
export const constants = statuses
