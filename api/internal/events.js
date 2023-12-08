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
   * Data associated with the `ApplicationURLEvent`.
   * @type {?any}
   */
  get data () { return this.#data ?? null }

  /**
   * The `URL` for the `ApplicationURLEvent`.
   * @type {?URL}
   */
  get url () {
    try {
      return this.#url ? new URL(this.#url) : null
    } catch (err) {
      return null
    }
  }
}

export default {
  ApplicationURLEvent
}
