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
  #parent
  #children = new Set()

  static constants = statuses

  constructor ({ index, ...options }) {
    this.#index = index
    this.#options = options
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

  getStatus () {
    return this.#options.status
  }

  async open () {
  }

  async close () {
    const { data: { status } } = await ipc.send('window.close', { index: globalThis.__args.index, targetWindowIndex: this.#index })
    return status
  }

  async kill () {
    const { data: { status } } = await ipc.send('window.kill', { index: globalThis.__args.index, targetWindowIndex: this.#index })
    return status
  }

  async exit () {
  }

  /**
   * @return {Promise<ipc.Result>}
   */
  async show () {
    return await ipc.send('window.show', { window: this.#index })
  }

  /**
   * @return {Promise<ipc.Result>}
   */
  async hide () {
    return await ipc.send('window.hide', { window: this.#index })
  }

  /**
   * @param {object} opts - an options object
   * @param {number=} [opts.window = currentWindow] - the index of the window
   * @param {string=} opts.url - the path to the HTML file to load into the window
   * @param {number=} opts.width - the width of the window
   * @param {number=} opts.height - the height of the window
   * @return {Promise<ipc.Result>}
   */
  async update (opts = {}) {
    // TODO(@chicoxyzzy): refactor; ex-show
    opts.index = this.#index
    opts.window ??= this.#index
    if (opts.url) {
      opts.url = formatFileUrl(opts.url)
    }
    return await ipc.send('window.show', opts)
  }

  async setTitile (title) {
    // TODO(@chicoxyzzy): rrefactor to send title AND window index
    return await ipc.send('window.setTitle', title)
  }

  /**
   * @param {object} url - file path
   * @return {Promise<ipc.Result>}
   */
  async navigate (url) {
    // TODO(@chicoxyzzy): rename url to path in the onMessage handler
    return await ipc.send('window.navigate', { window: this.#index, url: formatFileUrl(url) })
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
