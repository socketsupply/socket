/* global MessageEvent */
import application from '../application.js'

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
    const protocol = application.config.meta_application_protocol
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
   * @param {object=} [options]
   */
  constructor (type = 'hotkey', options = null) {
    super(type, { data: options })
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

export default {
  ApplicationURLEvent,
  HotKeyEvent
}
