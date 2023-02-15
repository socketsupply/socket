/**
 * @module Application
 *
 * Provides Application level methods
 */

import { applyPolyfills } from './polyfills.js'
import ipc, { primordials } from './ipc.js'
import ApplicationWindow, { formatFileUrl } from './window.js'
import { isValidPercentageValue } from './util.js'

import * as exports from './application.js'

if (globalThis.window) {
  applyPolyfills(globalThis.window)
}

/**
 * Creates a new window and returns an instance of ApplicationWindow.
 * @param {object} opts - an options object
 * @param {number=} [opts.index] - the index of the window
 * @param {string} opts.path - the path to the HTML file to load into the window
 * @param {string=} opts.title - the title of the window
 * @param {(number|string)=} [opts.width = '100%'] - the width of the window
 * @param {(number|string)=} [opts.height = '100%'] - the height of the window
 * @param {boolean=} [opts.resizable=true] - whether the window is resizable
 * @param {boolean=} [opts.frameless=false] - whether the window is frameless
 * @param {boolean=} [opts.utility=false] - whether the window is utility (macOS only)
 * @param {boolean=} [opts.canExit=false] - whether the window can exit the app
 * @return {Promise<ipc.Result>}
 */
export async function createWindow (opts) {
  if (opts.path === undefined) {
    throw new Error('Window path must be provided')
  }

  let width = Number(window.__args.config.width)
  let height = Number(window.__args.config.height)
  let isWidthInPercent = Boolean(window.__args.config.isWidthInPercent)
  let isHeightInPercent = Boolean(window.__args.config.isHeightInPercent)

  if ((opts.width != null && typeof opts.width !== 'number' && typeof opts.width !== 'string') ||
    (typeof opts.width === 'string' && !isValidPercentageValue(opts.width)) ||
    (typeof opts.width === 'number' && !(Number.isInteger(opts.width) && opts.width > 0))) {
    throw new Error(`Window width must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.width} instead.`)
  }
  if (typeof opts.width === 'string' && isValidPercentageValue(opts.width)) {
    width = Number(opts.width.slice(0, -1))
    isWidthInPercent = true
  }
  if ((opts.height != null && typeof opts.height !== 'number' && typeof opts.height !== 'string') ||
    (typeof opts.height === 'string' && !isValidPercentageValue(opts.height)) ||
    (typeof opts.height === 'number' && !(Number.isInteger(opts.height) && opts.height > 0))) {
    throw new Error(`Window height must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${opts.height} instead.`)
  }
  if (typeof opts.height === 'string' && isValidPercentageValue(opts.height)) {
    height = Number(opts.height.slice(0, -1))
    isHeightInPercent = true
  }

  const { data, err } = await ipc.send('window.create', {
    targetWindowIndex: opts.index,
    path: formatFileUrl(opts.path),
    index: globalThis.__args.index,
    title: opts.title ?? '',
    width: width,
    height: height,
    isWidthInPercent: isWidthInPercent,
    isHeightInPercent: isHeightInPercent,
    resizable: opts.resizable ?? true,
    frameless: opts.frameless ?? false,
    utility: opts.utility ?? false,
    canExit: opts.canExit ?? false
  })

  if (err) {
    throw new Error(err)
  }

  return new ApplicationWindow(data)
}

/**
 * Returns the current screen size.
 * @returns {Promise<ipc.Result>}
 */
export async function getScreenSize () {
  return await ipc.send('getScreenSize', {})
}

function throwOnInvalidIndex (index) {
  if (index === undefined || typeof index !== 'number' || !Number.isInteger(index) || index < 0) {
    throw new Error(`Invalid window index: ${index} (must be a positive integer number)`)
  }
}

/**
 * Returns the ApplicationWindow instances for the given indices or all windows if no indices are provided.
 * @param {number[]} indices - the indices of the windows
 * @return {Promise<ipc.Result>}
 * @throws {Error} - if indices is not an array of integer numbers
 */
export async function getWindows (indices) {
  let resultIndices = indices ?? []
  if (!Array.isArray(indices)) {
    throw new Error('Indices list must be an array of integer numbers')
  }
  for (const index of indices) {
    throwOnInvalidIndex(index)
  }
  const { data: windows } = await ipc.send('window.getWindows', resultIndices)
  return Object.fromEntries(windows.map(window => [Number(window.index), new ApplicationWindow(window)]))
}

/**
 * Returns the ApplicationWindow instance for the given index
 * @param {number} index - the index of the window
 * @return {Promise<ipc.Result>}
 * @throws {Error} - if index is not a valid integer number
 * @returns {Promise<ApplicationWindow | null>} - the ApplicationWindow instance or null if the window does not exist
 */
export async function getWindow (index) {
  throwOnInvalidIndex(index)
  const windows = await getWindows([index])
  return windows[index]
}

/**
 * Returns the ApplicationWindow instance for the current window.
 * @return {Promise<ApplicationWindow>}
 */
export async function getCurrentWindow () {
  return getWindow({ index: globalThis.__args.index })
}

/**
 * Quits the backend process and then quits the render process, the exit code used is the final exit code to the OS.
 * @param {object} options - an options object
 * @return {Promise<Any>}
 */
export async function exit (o) {
  return await ipc.send('exit', o)
}

/**
 * Set the native menu for the app.
 *
 * @param {object} options - an options object
 * @param {string} options.value - the menu layout
 * @param {number} options.index - the window to target (if applicable)
 * @return {Promise<Any>}
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
 * socket.runtime.setSystemMenu({ index: 0, value: `
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
  const menu = o.value

  // validate the menu
  if (typeof menu !== 'string' || menu.trim().length === 0) {
    throw new Error('Menu must be a non-empty string')
  }

  const menus = menu.match(/\w+:\n/g)
  if (!menus) {
    throw new Error('Menu must have a valid format')
  }
  const menuTerminals = menu.match(/;/g)
  const delta = menus.length - (menuTerminals?.length ?? 0)

  if ((delta !== 0) && (delta !== -1)) {
    throw new Error(`Expected ${menuTerminals.length} ';', found ${menus}.`)
  }

  const lines = menu.split('\n')
  const e = new Error()
  const frame = e.stack.split('\n')[2]
  const callerLineNo = frame.split(':').reverse()[1]

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i]
    const l = Number(callerLineNo) + i

    let errMsg

    if (line.trim().length === 0) continue
    if (/.*:\n/.test(line)) continue // ignore submenu labels
    if (/---/.test(line)) continue // ignore separators
    if (/\w+/.test(line) && !line.includes(':')) {
      errMsg = 'Missing label'
    } else if (/:\s*\+/.test(line)) {
      errMsg = 'Missing accelerator'
    } else if (/\+(\n|$)/.test(line)) {
      errMsg = 'Missing modifier'
    }

    if (errMsg) {
      throw new Error(`${errMsg} on line ${l}: "${line}"`)
    }
  }

  return await ipc.send('menu', o)
}

export async function setSystemMenuItemEnabled (value) {
  return await ipc.send('menuItemEnabled', value)
}

export const version = primordials.version

export default exports
