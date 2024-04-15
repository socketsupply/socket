// @ts-check
/**
 * @module Application
 *
 * Provides Application level methods
 *
 * Example usage:
 * ```js
 * import { createWindow } from 'socket:application'
 * ```
 */

import ApplicationWindow, { formatURL } from './window.js'
import { isValidPercentageValue } from './util.js'
import ipc, { primordials } from './ipc.js'
import menu, { setMenu } from './application/menu.js'
import os from './os.js'

import * as exports from './application.js'

function serializeConfig (config) {
  if (!config || typeof config !== 'object') {
    return ''
  }

  const entries = []
  for (const key in config) {
    entries.push(`${key} = ${config[key]}`)
  }

  return entries.join('\n')
}

export { menu }

// get this from constant value in runtime
export const MAX_WINDOWS = 32

export class ApplicationWindowList {
  #list = []

  static from (...args) {
    if (Array.isArray(args[0])) {
      return new this(args[0])
    }

    return new this(args)
  }

  constructor (items) {
    if (Array.isArray(items)) {
      for (const item of items) {
        this.add(item)
      }
    }
  }

  get length () {
    return this.#list.length
  }

  get size () {
    return this.length
  }

  get [Symbol.iterator] () {
    return this.#list[Symbol.iterator]
  }

  forEach (callback, thisArg) {
    this.#list.forEach(callback, thisArg)
  }

  item (index) {
    return this[index] ?? undefined
  }

  entries () {
    const entries = []

    for (const item of this.#list) {
      entries.push([item.index, item])
    }

    return entries
  }

  keys () {
    return this.entries().map((entry) => entry[0])
  }

  values () {
    return this.entries().map((entry) => entry[1])
  }

  add (window) {
    if (Number.isFinite(window.index) && window.index > -1) {
      this[window.index] = window

      for (let i = 0; i < this.#list.length; ++i) {
        if (this.#list[i].index === window.index) {
          this.#list.splice(i, 1)
          break
        }
      }

      this.#list.push(window)
      this.#list.sort((a, b) => a.index - b.index)
    }

    return this
  }

  remove (windowOrIndex) {
    let index = -1
    if (Number.isFinite(windowOrIndex) && windowOrIndex > -1) {
      index = windowOrIndex
    } else {
      index = windowOrIndex?.index ?? -1
    }

    if (index > -1) {
      delete this[index]
      for (let i = 0; i < this.#list.length; ++i) {
        if (this.#list[i].index === index) {
          this.#list.splice(i, 1)
          return true
        }
      }
    }

    return false
  }

  contains (windowOrIndex) {
    let index = -1
    if (Number.isFinite(windowOrIndex) && windowOrIndex > -1) {
      index = windowOrIndex
    } else {
      index = windowOrIndex?.index ?? -1
    }

    if (index > -1) {
      return Boolean(this[index])
    }

    return false
  }

  clear () {
    for (const item of this.#list) {
      delete this[item.index]
    }

    this.#list = []
    return this
  }
}

/**
 * Returns the current window index
 * @return {number}
 */
export function getCurrentWindowIndex () {
  return globalThis.__args.index ?? 0
}

/**
 * Creates a new window and returns an instance of ApplicationWindow.
 * @param {object} opts - an options object
 * @param {string=} opts.aspectRatio - a string (split on ':') provides two float values which set the window's aspect ratio.
 * @param {boolean=} opts.closable - deterime if the window can be closed.
 * @param {boolean=} opts.minimizable - deterime if the window can be minimized.
 * @param {boolean=} opts.maximizable - deterime if the window can be maximized.
 * @param {number} [opts.margin] - a margin around the webview. (Private)
 * @param {number} [opts.radius] - a radius on the webview. (Private)
 * @param {number} opts.index - the index of the window.
 * @param {string} opts.path - the path to the HTML file to load into the window.
 * @param {string=} opts.title - the title of the window.
 * @param {string=} opts.titlebarStyle - determines the style of the titlebar (MacOS only).
 * @param {string=} opts.windowControlOffsets - a string (split on 'x') provides the x and y position of the traffic lights (MacOS only).
 * @param {string=} opts.backgroundColorDark - determines the background color of the window in dark mode.
 * @param {string=} opts.backgroundColorLight - determines the background color of the window in light mode.
 * @param {(number|string)=} opts.width - the width of the window. If undefined, the window will have the main window width.
 * @param {(number|string)=} opts.height - the height of the window. If undefined, the window will have the main window height.
 * @param {(number|string)=} [opts.minWidth = 0] - the minimum width of the window
 * @param {(number|string)=} [opts.minHeight = 0] - the minimum height of the window
 * @param {(number|string)=} [opts.maxWidth = '100%'] - the maximum width of the window
 * @param {(number|string)=} [opts.maxHeight = '100%'] - the maximum height of the window
 * @param {boolean=} [opts.resizable=true] - whether the window is resizable
 * @param {boolean=} [opts.frameless=false] - whether the window is frameless
 * @param {boolean=} [opts.utility=false] - whether the window is utility (macOS only)
 * @param {boolean=} [opts.canExit=false] - whether the window can exit the app
 * @param {boolean=} [opts.headless=false] - whether the window will be headless or not (no frame)
 * @param {string=} [opts.userScript=null] - A user script that will be injected into the window (desktop only)
 * @param {string[]=} [opts.protocolHandlers] - An array of protocol handler schemes to register with the new window (requires service worker)
 * @return {Promise<ApplicationWindow>}
 */
export async function createWindow (opts) {
  if (typeof opts?.path !== 'string' || typeof opts?.index !== 'number') {
    throw new Error('Path and index are required options')
  }

  // default values
  const options = {
    targetWindowIndex: opts.index,
    url: formatURL(opts.path),
    index: globalThis.__args.index,
    title: opts.title ?? '',
    resizable: opts.resizable ?? true,
    closable: opts.closable === true,
    maximizable: opts.maximizable ?? true,
    minimizable: opts.minimizable ?? true,
    frameless: opts.frameless ?? false,
    aspectRatio: opts.aspectRatio ?? '',
    titlebarStyle: opts.titlebarStyle ?? '',
    windowControlOffsets: opts.windowControlOffsets ?? '',
    backgroundColorDark: opts.backgroundColorDark ?? '',
    backgroundColorLight: opts.backgroundColorLight ?? '',
    utility: opts.utility ?? false,
    canExit: opts.canExit ?? false,
    /**
     * @private
     * @type {number}
     */
    radius: opts.radius ?? 0,
    /**
     * @private
     * @type {number}
     */
    margin: opts.margin ?? 0,
    minWidth: opts.minWidth ?? 0,
    minHeight: opts.minHeight ?? 0,
    maxWidth: opts.maxWidth ?? '100%',
    maxHeight: opts.maxHeight ?? '100%',
    headless: opts.headless === true,
    // @ts-ignore
    debug: opts.debug === true, // internal
    userScript: encodeURIComponent(opts.userScript ?? ''),
    // @ts-ignore
    __runtime_primordial_overrides__: (
      // @ts-ignore
      opts.__runtime_primordial_overrides__ &&
      // @ts-ignore
      typeof opts.__runtime_primordial_overrides__ === 'object'
      // @ts-ignore
        ? JSON.stringify(opts.__runtime_primordial_overrides__)
        : ''
    ),
    // @ts-ignore
    config: typeof opts?.config === 'string'
      // @ts-ignore
      ? opts.config
      // @ts-ignore
      : (serializeConfig(opts?.config) ?? '')
  }

  if (Array.isArray(opts?.protocolHandlers)) {
    for (const protocolHandler of opts.protocolHandlers) {
      // @ts-ignore
      opts.config[`webview_protocol-handlers_${protocolHandler}`] = ''
    }
  } else if (opts?.protocolHandlers && typeof opts.protocolHandlers === 'object') {
    // @ts-ignore
    for (const key in opts.protocolHandlers) {
      // @ts-ignore
      if (opts.protocolHandlers[key] && typeof opts.protocolHandlers[key] === 'object') {
        // @ts-ignore
        opts.config[`webview_protocol-handlers_${key}`] = JSON.stringify(opts.protocolHandlers[key])
        // @ts-ignore
      } else if (typeof opts.protocolHandlers[key] === 'string') {
        // @ts-ignore
        opts.config[`webview_protocol-handlers_${key}`] = opts.protocolHandlers[key]
      }
    }
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

  const { data, err } = await ipc.send('window.create', options)

  if (err) {
    throw err
  }

  return new ApplicationWindow(data)
}

/**
 * Returns the current screen size.
 * @returns {Promise<{ width: number, height: number }>}
 */
export async function getScreenSize () {
  if (os.platform() === 'ios' || os.platform() === 'android') {
    return {
      width: globalThis.screen?.availWidth ?? 0,
      height: globalThis.screen?.availHeight ?? 0
    }
  }
  const { data, err } = await ipc.send('application.getScreenSize', { index: globalThis.__args.index })
  if (err) {
    throw err
  }
  return data
}

function throwOnInvalidIndex (index) {
  if (index === undefined || typeof index !== 'number' || !Number.isInteger(index) || index < 0) {
    throw new Error(`Invalid window index: ${index} (must be a positive integer number)`)
  }
}

/**
 * Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.
 * @param {number[]} [indices] - the indices of the windows
 * @throws {Error} - if indices is not an array of integer numbers
 * @return {Promise<ApplicationWindowList>}
 */
export async function getWindows (indices, options = null) {
  if (
    !globalThis.RUNTIME_APPLICATION_ALLOW_MULTI_WINDOWS &&
    (os.platform() === 'ios' || os.platform() === 'android')
  ) {
    return new ApplicationWindowList([
      new ApplicationWindow({
        index: 0,
        id: globalThis.__args?.client?.id ?? null,
        width: globalThis.screen?.availWidth ?? 0,
        height: globalThis.screen?.availHeight ?? 0,
        title: document.title,
        status: 31
      })
    ])
  }

  // TODO: create a local registry and return from it when possible
  const resultIndices = indices ?? []

  if (!Array.isArray(resultIndices)) {
    throw new Error('Indices list must be an array of integer numbers')
  }

  for (const index of resultIndices) {
    throwOnInvalidIndex(index)
  }

  const result = await ipc.send('application.getWindows', resultIndices)

  if (result.err) {
    throw result.err
  }

  // 0 indexed based key to `ApplicationWindow` object map
  const windows = new ApplicationWindowList()

  if (!Array.isArray(result.data)) {
    return windows
  }

  for (const data of result.data) {
    const max = Number.isFinite(options?.max) ? options.max : MAX_WINDOWS
    if (options?.max === false || data.index < max) {
      windows.add(new ApplicationWindow(data))
    }
  }

  return windows
}

/**
 * Returns the ApplicationWindow instance for the given index
 * @param {number} index - the index of the window
 * @throws {Error} - if index is not a valid integer number
 * @returns {Promise<ApplicationWindow>} - the ApplicationWindow instance or null if the window does not exist
 */
export async function getWindow (index, options) {
  throwOnInvalidIndex(index)
  const windows = await getWindows([index], options)
  return windows[index]
}

/**
 * Returns the ApplicationWindow instance for the current window.
 * @return {Promise<ApplicationWindow>}
 */
export async function getCurrentWindow () {
  return await getWindow(globalThis.__args.index, { max: false })
}

/**
 * Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.
 * @param {number} [code = 0] - an exit code
 * @return {Promise<ipc.Result>}
 */
export async function exit (code = 0) {
  const { data, err } = await ipc.send('application.exit', code)
  if (err) {
    throw err
  }
  return data
}

/**
 * Set the native menu for the app.
 *
 * @param {object} options - an options object
 * @param {string} options.value - the menu layout
 * @param {number} options.index - the window to target (if applicable)
 * @return {Promise<ipc.Result>}
 *
 * Socket Runtime provides a minimalist DSL that makes it easy to create
 * cross platform native system and context menus.
 *
 * Menus are created at run time. They can be created from either the Main or
 * Render process. The can be recreated instantly by calling the `setSystemMenu` method.
 *
 * The method takes a string. Here's an example of a menu. The semi colon is
 * significant indicates the end of the menu. Use an underscore when there is no
 * accelerator key. Modifiers are optional. And well known OS menu options like
 * the edit menu will automatically get accelerators you dont need to specify them.
 *
 *
 * ```js
 * socket.application.setSystemMenu({ index: 0, value: `
 *   App:
 *     Foo: f;
 *
 *   Edit:
 *     Cut: x
 *     Copy: c
 *     Paste: v
 *     Delete: _
 *     Select All: a;
 *
 *   Other:
 *     Apple: _
 *     Another Test: T
 *     !Im Disabled: I
 *     Some Thing: S + Meta
 *     ---
 *     Bazz: s + Meta, Control, Alt;
 * `)
 * ```
 *
 * Separators
 *
 * To create a separator, use three dashes `---`.
 *
 *
 * Accelerator Modifiers
 *
 * Accelerator modifiers are used as visual indicators but don't have a
 * material impact as the actual key binding is done in the event listener.
 *
 * A capital letter implies that the accelerator is modified by the `Shift` key.
 *
 * Additional accelerators are `Meta`, `Control`, `Option`, each separated
 * by commas. If one is not applicable for a platform, it will just be ignored.
 *
 * On MacOS `Meta` is the same as `Command`.
 *
 *
 * Disabled Items
 *
 * If you want to disable a menu item just prefix the item with the `!` character.
 * This will cause the item to appear disabled when the system menu renders.
 *
 *
 * Submenus
 *
 * We feel like nested menus are an anti-pattern. We don't use them. If you have a
 * strong argument for them and a very simple pull request that makes them work we
 * may consider them.
 *
 *
 * Event Handling
 *
 * When a menu item is activated, it raises the `menuItemSelected` event in
 * the front end code, you can then communicate with your backend code if you
 * want from there.
 *
 * For example, if the `Apple` item is selected from the `Other` menu...
 *
 * ```js
 * window.addEventListener('menuItemSelected', event => {
 *   assert(event.detail.parent === 'Other')
 *   assert(event.detail.title === 'Apple')
 * })
 * ```
 *
 */
export async function setSystemMenu (o) {
  return await setMenu(o, 'system')
}

/**
 * An alias to setSystemMenu for creating a tary menu
 */
export async function setTrayMenu (o) {
  return await setMenu(o, 'tray')
}

/**
 * Set the enabled state of the system menu.
 * @param {object} value - an options object
 * @return {Promise<ipc.Result>}
 */
export async function setSystemMenuItemEnabled (value) {
  return await ipc.send('application.setSystemMenuItemEnabled', value)
}

/**
 * Socket Runtime version.
 * @type {object} - an object containing the version information
 */
export const runtimeVersion = primordials.version

/**
 * Runtime debug flag.
 * @type {boolean}
 */
export const debug = !!globalThis.__args?.debug

/**
 * Application configuration.
 * @type {object}
 */
export const config = globalThis.__args?.config ?? {}

/**
 * The application's backend instance.
 */
export const backend = {
  /**
   * @param {object} opts - an options object
   * @param {boolean} [opts.force = false] - whether to force the existing process to close
   * @return {Promise<ipc.Result>}
   */
  async open (opts = {}) {
    opts.force ??= false
    return await ipc.send('process.open', opts)
  },

  /**
   * @return {Promise<ipc.Result>}
   */
  async close () {
    return await ipc.send('process.kill')
  }
}

export default exports
