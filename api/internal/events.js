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

export default {
  ApplicationURLEvent
}
