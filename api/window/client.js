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
   * @param {ClientState} state
   */
  constructor (state) {
    this.#state = state
  }

  get id () {
    return this.#state?.id ?? null
  }

  get frameType () {
    return this.#state?.frameType ?? 'none'
  }

  get type () {
    return this.#state?.type ?? ''
  }

  get parent () {
    return this.#state?.parent ?
      new Client(this.#state.parent)
      : null
  }

  get top () {
    return this.#state?.top
      ? new Client(this.#state.top)
      : null
  }
}

export default new Client (globalThis.__args?.client ?? {})
