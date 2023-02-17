/**
 * @module Window
 *
 * Provides Window class and methods
 */

import * as statuses from './window/constants.js'
import ipc, { primordials } from './ipc.js'

export function formatFileUrl (url) {
  return `file://${primordials.cwd}/${url}`
}

// TODO(@chicoxyzzy): JSDoc
export class ApplicationWindow {
  #index
  #options
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
      index: globalThis.__args.index,
      targetWindowIndex: this.#index
    })
    if (err) {
      throw new Error(err)
    }
    return data
  }

  async kill () {
    const { data, err } = await ipc.send('window.kill', {
      index: globalThis.__args.index,
      targetWindowIndex: this.#index
    })
    if (err) {
      throw new Error(err)
    }
    return data
  }

  async exit (code) {
    const { data, err } = await ipc.send('window.exit', {
      index: globalThis.__args.index,
      targetWindowIndex: this.#index,
      code
    })
    if (err) {
      throw new Error(err)
    }
    return data
  }

  /**
   * @return {Promise<ipc.Result>}
   */
  async show () {
    const response = await ipc.send('window.show', { index: this.#index, targetWindowIndex: this.#index })
    return this.#updateOptions(response)
  }

  /**
   * @return {Promise<ipc.Result>}
   */
  async hide () {
    const response = await ipc.send('window.hide', { index: this.#index, targetWindowIndex: this.#index })
    return this.#updateOptions(response)
  }

  async setTitle (title) {
    const response = await ipc.send('window.setTitle', { index: this.#index, targetWindowIndex: this.#index, value: title })
    return this.#updateOptions(response)
  }

  /**
   * @param {object} path - file path
   * @return {Promise<ipc.Result>}
   */
  async navigate (path) {
    const response = await ipc.send('window.navigate', { index: this.#index, targetWindowIndex: this.#index, url: formatFileUrl(path) })
    return this.#updateOptions(response)
  }

  /**
   * @param {object} opts - an options object
   * @param {number} opts.red - the red value
   * @param {number} opts.green - the green value
   * @param {number} opts.blue - the blue value
   * @param {number} opts.alpha - the alpha value
   * @return {Promise<ipc.Result>}
   */
  async setBackgrounColor (opts) {
    opts.index = this.#index
    const o = new URLSearchParams(opts).toString()
    await ipc.postMessage(`ipc://window.setBackgroundColor?${o}`)
  }

  /**
   * Opens a native context menu.
   * @param {object} options - an options object
   * @return {Promise<Any>}
   */
  async setContextMenu (o) {
    o = Object
      .entries(o)
      .flatMap(a => a.join(':'))
      .join('_')
    return await ipc.send('context', o)
  }

  async reload () {
    // TODO(@chicoxyzzy): which window to reload?
    ipc.postMessage('ipc://reload')
  }

  /**
   * @param {object} options - an options object
   * @param {number=} [options.window=currentWindow] - the window to send the message to
   * @param {string} options.event - the event to send
   * @param {(string|object)=} options.value - the value to send
   * @returns
   */
  async send (options) {
    options.index = this.#index
    options.window ??= -1

    if (typeof options.value !== 'string') {
      options.value = JSON.stringify(options.value)
    }

    return await ipc.send('send', {
      index: options.index,
      window: options.window,
      event: encodeURIComponent(options.event),
      value: encodeURIComponent(options.value)
    })
  }

  /**
   *
   * @param {object} options
   * @returns {Promise<ipc.Result>}
   */
  async openExternal (options) {
    return await ipc.postMessage(`ipc://external?value=${encodeURIComponent(options)}`)
  }

  // TODO(@chicoxyzzy): implement on method
}

export default ApplicationWindow
export const constants = statuses
