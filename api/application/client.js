import location from '../location.js'

/**
 * @typedef {{
 *  id?: string | null,
 *  type?: 'window' | 'worker',
 *  parent?: object | null,
 *  top?: object | null,
 *  frameType?: 'top-level' | 'nested' | 'none'
 * }} ClientState
 */

export class Client {
  /**
   * @type {ClientState}
   */
  #state = null

  /**
   * `Client` class constructor
   * @private
   * @param {ClientState} state
   */
  constructor (state) {
    this.#state = state
  }

  /**
   * The unique ID of the client.
   * @type {string|null}
   */
  get id () {
    return this.#state?.id ?? null
  }

  /**
   * The frame type of the client.
   * @type {'top-level'|'nested'|'none'}
   */
  get frameType () {
    return this.#state?.frameType ?? 'none'
  }

  /**
   * The type of the client.
   * @type {'window'|'worker'}
   */
  get type () {
    return this.#state?.type ?? ''
  }

  /**
   * The parent client of the client.
   * @type {Client|null}
   */
  get parent () {
    return this.#state?.parent
      ? new Client(this.#state.parent)
      : null
  }

  /**
   * The top client of the client.
   * @type {Client|null}
   */
  get top () {
    return this.#state?.top
      ? new Client(this.#state.top)
      : null
  }

  /**
   * A readonly `URL` of the current location of this client.
   * @type {URL}
   */
  get location () {
    return new URL(globalThis.RUNTIME_WORKER_LOCATION ?? location.href)
  }

  /**
   * Converts this `Client` instance to JSON.
   * @return {object}
   */
  toJSON () {
    return {
      id: this.id,
      frameType: this.frameType,
      type: this.type,
      location: this.location.toString()
    }
  }
}

// @ts-ignore
export default new Client(globalThis.__args?.client ?? {})
