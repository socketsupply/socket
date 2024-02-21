/* global Event, MessageEvent */

/**
 * An event dispatched when an application URL is opening the application.
 */
export class ApplicationURLEvent extends Event {
  #data = null
  #url = null

  /**
   * `ApplicationURLEvent` class constructor.
   * @param {string=} [type]
   * @param {object=} [options]
   */
  constructor (type = 'applicationurl', options = null) {
    super(type)
    this.#data = options?.data ?? null
    this.#url = options?.url ?? null

    if (this.#url !== null && typeof this.#url === 'object') {
      this.#url = this.#url.toString()
    }
  }

  /**
   * String tag name for an `ApplicationURLEvent` instance.
   * @type {string}
   */
  get [Symbol.toStringTag] () {
    return 'ApplicationURLEvent'
  }

  /**
   * @ignore
   * @type {boolean}
   */
  get isTrusted () { return true }

  /**
   * `true` if the application URL is valid (parses correctly).
   * @type {boolean}
   */
  get isValid () { return this.url !== null }

  /**
   * Data associated with the `ApplicationURLEvent`.
   * @type {?any}
   */
  get data () { return this.#data ?? null }

  /**
   * The original source URI
   * @type {?string}
   */
  get source () { return this.#url ?? null }

  /**
   * The `URL` for the `ApplicationURLEvent`.
   * @type {?URL}
   */
  get url () {
    const protocol = globalThis.__args.config.meta_application_protocol ?? ''
    let { source } = this

    if (!source) {
      return null
    }

    if (source.startsWith(`${protocol}:`) && !source.startsWith(`${protocol}://`)) {
      source = source.replace(`${protocol}:`, `${protocol}://`)
    }

    try {
      return new URL(source)
    } catch (err) {
      return null
    }
  }
}

/**
 * An event dispacted for a registered global hotkey expression.
 */
export class HotKeyEvent extends MessageEvent {
  /**
   * `HotKeyEvent` class constructor.
   * @ignore
   * @param {string=} [type]
   * @param {object=} [data]
   */
  constructor (type = 'hotkey', data = null) {
    super(type, { data })
  }

  /**
   * The global unique ID for this hotkey binding.
   * @type {number?}
   */
  get id () {
    return this.data?.id ?? 0
  }

  /**
   * The computed hash for this hotkey binding.
   * @type {number?}
   */
  get hash () {
    return this.data?.hash ?? 0
  }

  /**
   * The normalized hotkey expression as a sequence of tokens.
   * @type {string[]}
   */
  get sequence () {
    return this.data?.sequence ?? []
  }

  /**
   * The original expression of the hotkey binding.
   * @type {string?}
   */
  get expression () {
    return this.data?.expression ?? null
  }
}

/**
 * An event dispacted when a menu item is selected.
 */
export class MenuItemEvent extends MessageEvent {
  #menu = null

  /**
   * `MenuItemEvent` class constructor
   * @ignore
   * @param {string=} [type]
   * @param {object=} [data]
   * @param {import('../application/menu.js').Menu} menu
   */
  constructor (type = 'menuitem', data = null, menu = null) {
    super(type, {
      data: {
        type: data?.type ?? null,
        title: data?.title ?? null,
        parent: data?.parent ?? null
      }
    })

    this.#menu = menu
  }

  /**
   * The `Menu` this event has been dispatched for.
   * @type {import('../application/menu.js').Menu?}
   */
  get menu () {
    return this.#menu
  }

  /**
   * The title of the menu item.
   * @type {string?}
   */
  get title () {
    return this.data?.title ?? null
  }

  /**
   * An optional tag value for the menu item that may also be the
   * parent menu item title.
   * @type {string?}
   */
  get tag () {
    return this.data?.parent
  }

  /**
   * The parent title of the menu item.
   * @type {string?}
   */
  get parent () {
    if (this.menu?.type !== 'system') {
      return null
    }

    return this.data?.parent ?? null
  }
}

/**
 * An event dispacted when the application receives an OS signal
 */
export class SignalEvent extends MessageEvent {
  #code = 0
  #name = ''
  #message = ''

  /**
   * `SignalEvent` class constructor
   * @ignore
   * @param {string=} [type]
   * @param {object=} [options]
   */
  constructor (type, options) {
    super(type, options)
    this.#code = options?.code ?? 0
    this.#name = type
    this.#message = options?.message ?? ''
  }

  /**
   * The code of the signal.
   * @type {import('../signal.js').signal}
   */
  get code () {
    return this.#code ?? 0
  }

  /**
   * The name of the signal.
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * An optional message describing the signal
   * @type {string}
   */
  get message () {
    return this.#message ?? ''
  }
}
export default {
  ApplicationURLEvent,
  MenuItemEvent,
  SignalEvent,
  HotKeyEvent
}
