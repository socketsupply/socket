/* global ErrorEvent */
import { MenuItemEvent } from '../internal/events.js'
import { Deferred } from '../async.js'
import ipc from '../ipc.js'

let contextMenuDeferred = null

/**
 * Helper for getting the current window index.
 * @ignore
 * @return {number}
 */
function getCurrentWindowIndex () {
  return globalThis.__args.index ?? 0
}

/**
 * A `Menu` is base class for a `ContextMenu`, `SystemMenu`, or `TrayMenu`.
 */
export class Menu extends EventTarget {
  /**
   * The `Menu` instance type.
   * @type {('context'|'system'|'tray')?}
   */
  #type = null

  /**
   * The `BroadcastChannel` for this `Menu` instance so all windows
   * can propagate menu events.
   * @type {BroadcastChannel}
   */
  #channel = null

  /**
   * Level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  #onerror = null

  /**
   * Level 1 'menuitem'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  #onmenuitem = null

  /**
   * `Menu` class constructor.
   * @ignore
   * @param {string} type
   */
  constructor (type) {
    super()
    this.#type = type
    this.#channel = new BroadcastChannel(`socket.runtime.application.menu.${type}`)
    this.#channel.addEventListener('message', (event) => {
      this.dispatchEvent(new MenuItemEvent('menuitem', event.data, this))
    })
  }

  /**
   * The broadcast channel for this menu.
   * @ignore
   * @type {BroadcastChannel}
   */
  get channel () {
    return this.#channel
  }

  /**
   * The `Menu` instance type.
   * @type {('context'|'system'|'tray')?}
   */
  get type () {
    return this.#type ?? null
  }

  /**
   * Level 1 'error'` event listener.
   * @type {function(ErrorEvent)?}
   */
  get onerror () {
    return this.#onerror ?? null
  }

  /**
   * Setter for the level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  set onerror (onerror) {
    if (this.#onerror) {
      this.removeEventListener('error', this.#onerror)
    }

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  /**
   * Level 1 'menuitem'` event listener.
   * @type {function(menuitemEvent)?}
   */
  get onmenuitem () {
    return this.#onmenuitem ?? null
  }

  /**
   * Setter for the level 1 'menuitem'` event listener.
   * @ignore
   * @type {function(MenuItemEvent)?}
   */
  set onmenuitem (onmenuitem) {
    if (this.#onmenuitem) {
      this.removeEventListener('menuitem', this.#onmenuitem)
    }

    if (typeof onmenuitem === 'function') {
      this.#onmenuitem = onmenuitem
      this.addEventListener('menuitem', onmenuitem)
    }
  }

  /**
   * Set the menu layout for this `Menu` instance.
   * @param {string|object} layoutOrOptions
   * @param {object=} [options]
   */
  async set (layoutOrOptions, options = null) {
    if (this.type === 'context') {
      if (Array.isArray(layoutOrOptions)) {
        const object = {}

        for (const item of layoutOrOptions) {
          object[item] = true
        }

        layoutOrOptions = object
      } else if (typeof layoutOrOptions === 'string') {
        const object = {}
        const lines = layoutOrOptions.split('\n')

        for (const line of lines) {
          const [key, value] = line.trim().split(':').map((value) => value.trim())
          object[key] = value
        }

        layoutOrOptions = object
      }

      options = /** @type {object} */ (layoutOrOptions)
      const result = await setContextMenu(options)

      if (result.err) {
        throw result.err
      }

      return result.data
    }

    if (typeof layoutOrOptions === 'object') {
      const descriptor = layoutOrOptions
      const buffer = []

      function visit (value, indent = 0) {
        const padding = ''.padStart(indent)
        if (typeof value === 'string') {
          buffer.push(value)
        } else if (value && typeof value === 'object') {
          buffer.push(padding, '\n')
          for (const key in value) {
            const v = value[key]
            buffer.push(padding, `${key}:\n`)
            visit(v, indent + 2)
            buffer.push(padding, ';', '\n')
          }
        }
      }

      if (Array.isArray(layoutOrOptions)) {
        for (const item of layoutOrOptions) {
          if (item && typeof item === 'object') {
            for (const key in item) {
              buffer.push(`${key}: ${item};\n`)
            }
          }
        }
      } else {
        for (const key in descriptor) {
          const value = descriptor[key]
          buffer.push(`${key}: `)
          visit(value, 2)
          buffer.push(';', '\n')
        }
      }

      layoutOrOptions = buffer.join('').trim()
    }

    const layout = /** @type {string} */ (layoutOrOptions)
    const result = await setMenu({
      ...options,
      index: getCurrentWindowIndex(),
      value: layout
    }, this.type)

    if (result.err) {
      throw result.err
    }

    return result.data
  }
}

/**
 * A container for various `Menu` instances.
 */
export class MenuContainer extends EventTarget {
  /**
   * The tray menu for this container.
   * @type {TrayMenu}
   */
  #tray = null

  /**
   * The system menu for this container.
   * @type {SystemMenu}
   */
  #system = null

  /**
   * The context menu for this container.
   * @type {ContextMenu}
   */
  #context = null

  /**
   * Level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  #onerror = null

  /**
   * Level 1 'menuitem'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  #onmenuitem = null

  /**
   * `MenuContainer` class constructor.
   * @param {EventTarget} [sourceEventTarget]
   * @param {object=} [options]
   */
  constructor (sourceEventTarget = globalThis, options = null) {
    super()

    this.#tray = options?.tray ?? null
    this.#system = options?.system ?? null
    this.#context = options?.context ?? null

    sourceEventTarget.addEventListener('menuItemSelected', (event) => {
      if (contextMenuDeferred) {
        contextMenuDeferred.resolve({ data: event.detail.parent })
      }

      const detail = event.detail ?? {}
      const menu = this[detail.type ?? '']

      if (menu) {
        menu.dispatchEvent(new MenuItemEvent('menuitem', detail, menu))
        menu.channel.postMessage({
          ...detail,
          source: {
            window: {
              index: getCurrentWindowIndex()
            }
          }
        })
      }
    })

    if (this.#tray) {
      this.#tray.addEventListener('menuitem', (event) => {
        this.dispatchEvent(new MenuItemEvent(event.type, event.data, this.#tray))
      })

      this.#tray.addEventListener('error', (event) => {
        this.dispatchEvent(new ErrorEvent(event.type, event))
      })
    }

    if (this.#system) {
      this.#system.addEventListener('menuitem', (event) => {
        this.dispatchEvent(new MenuItemEvent(event.type, event.data, this.#system))
      })

      this.#system.addEventListener('error', (event) => {
        this.dispatchEvent(new ErrorEvent(event.type, event))
      })
    }

    if (this.#context) {
      this.#context.addEventListener('menuitem', (event) => {
        this.dispatchEvent(new MenuItemEvent(event.type, event.data, this.#context))
      })

      this.#context.addEventListener('error', (event) => {
        this.dispatchEvent(new ErrorEvent(event.type, event))
      })
    }
  }

  /**
   * Level 1 'error'` event listener.
   * @type {function(ErrorEvent)?}
   */
  get onerror () {
    return this.#onerror ?? null
  }

  /**
   * Setter for the level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  set onerror (onerror) {
    if (this.#onerror) {
      this.removeEventListener('error', this.#onerror)
    }

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  /**
   * Level 1 'menuitem'` event listener.
   * @type {function(menuitemEvent)?}
   */
  get onmenuitem () {
    return this.#onmenuitem ?? null
  }

  /**
   * Setter for the level 1 'menuitem'` event listener.
   * @ignore
   * @type {function(MenuItemEvent)?}
   */
  set onmenuitem (onmenuitem) {
    if (this.#onmenuitem) {
      this.removeEventListener('menuitem', this.#onmenuitem)
    }

    if (typeof onmenuitem === 'function') {
      this.#onmenuitem = onmenuitem
      this.addEventListener('menuitem', onmenuitem)
    }
  }

  /**
   * The `TrayMenu` instance for the application.
   * @type {TrayMenu}
   */
  get tray () {
    return this.#tray
  }

  /**
   * The `SystemMenu` instance for the application.
   * @type {SystemMenu}
   */
  get system () {
    return this.#system
  }

  /**
   * The `ContextMenu` instance for the application.
   * @type {ContextMenu}
   */
  get context () {
    return this.#context
  }
}

/**
 * A `Menu` instance that represents a context menu.
 */
export class ContextMenu extends Menu {
  constructor () {
    super('context')
  }
}

/**
 * A `Menu` instance that represents the system menu.
 */
export class SystemMenu extends Menu {
  constructor () {
    super('system')
  }
}

/**
 * A `Menu` instance that represents the tray menu.
 */
export class TrayMenu extends Menu {
  constructor () {
    super('tray')
  }
}

/**
 * The application tray menu.
 * @type {TrayMenu}
 */
export const tray = new TrayMenu()

/**
 * The application system menu.
 * @type {SystemMenu}
 */
export const system = new SystemMenu()

/**
 * The application context menu.
 * @type {ContextMenu}
 */
export const context = new ContextMenu()

/**
 * The application menus container.
 * @type {MenuContainer}
 */
export const container = new MenuContainer(globalThis, {
  context,
  system,
  tray
})

export default container

/**
 * Internal IPC for setting an application menu
 * @ignore
 */
export async function setMenu (options, type) {
  const menu = options.value

  // validate the menu
  if (typeof menu !== 'string' || menu.trim().length === 0) {
    throw new Error('Menu must be a non-empty string')
  }

  const menus = type === 'tray'
    ? menu.match(/\w+:?\n?/g)
    : menu.match(/\w+:\n/g)

  if (!menus) {
    throw new Error('Menu must have a valid format')
  }

  // validate a 'system' menu type syntax
  if (type === 'system') {
    const menuTerminals = menu.match(/;/g)
    const delta = menus.length - (menuTerminals?.length ?? 0)

    if ((delta !== 0) && (delta !== -1)) {
      throw new Error(`Expected ${menuTerminals.length} ';', found ${menus}.`)
    }

    const lines = menu.split('\n')
    const e = new Error()
    const frame = e.stack.split('\n')[2]
    const callerLineNo = frame.split(':').reverse()[1]

    // Use this link to test the regex (https://regexr.com/7lhqe)
    const validLineRegex = /^(?:([^:]+)|(.+)[:][ ]*((?:[+\w]+(?:[ ]+|[ ]*$))*.*))$/m
    const validModifiers = /^(Alt|Option|CommandOrControl|Control|Meta|Super)$/i

    for (let i = 0; i < lines.length; i++) {
      const lineText = lines[i].trim()
      if (lineText.length === 0) {
        continue // Empty line
      }

      if (lineText[0] === ';') {
        continue // End of submenu
      }

      let err

      const match = lineText.match(validLineRegex)
      if (!match) {
        err = 'Unsupported syntax'
      } else {
        const label = match[1] || match[2]
        if (label.startsWith('---')) {
          continue // Valid separator
        }
        const binding = match[3]
        if (label.length === 0) {
          err = 'Missing label'
        } else if (label.includes(':')) {
          err = 'Invalid label contains ":"'
        } else if (binding) {
          const [accelerator, modifiersRaw] = binding.split(/ *\+ */)
          const modifiers = modifiersRaw?.replace(';', '').split(', ') ?? []
          if (validModifiers.test(accelerator)) {
            err = 'Missing accelerator'
          } else {
            for (const modifier of modifiers) {
              if (!validModifiers.test(modifier)) {
                err = `Invalid modifier "${modifier}"`
                break
              }
            }
          }
        }
      }

      if (err) {
        const lineNo = Number(callerLineNo) + i
        return ipc.Result.from({ err: new Error(`${err} on line ${lineNo}: "${lineText}"`) })
      }
    }
  }

  const command = (
    type === 'tray'
      ? 'application.setTrayMenu'
      : 'application.setSystemMenu'
  )

  return await ipc.send(command, options)
}

/**
 * Internal IPC for setting an application context menu
 * @ignore
 */
export async function setContextMenu (options) {
  const lines = options.value.split('\n')
  const e = new Error()
  const frame = e.stack.split('\n')[2]
  const callerLineNo = frame.split(':').reverse()[1]

  let err
  let lineText

  for (let i = 0; i < lines.length; i++) {
    lineText = lines[i].trim()

    if (!lineText.length) continue
    if (lineText.includes('---')) continue
    if (!lineText.includes(':')) {
      err = 'Expected separator (:)'
    }

    const parts = lineText.split(':')
    if (!parts[0].trim()) {
      err = 'Expected label'
    }

    if (!parts[0].trim()) {
      err = 'Expected accelerator'
    }

    if (err) {
      const lineNo = Number(callerLineNo) + i
      return ipc.Result.from({ err: new Error(`${err} on line ${lineNo}: "${lineText}"`) })
    }
  }

  contextMenuDeferred = new Deferred()

  const result = await ipc.send('window.setContextMenu', options)

  if (result && result.err) {
    return { err: result.err }
  }

  return await contextMenuDeferred
}
