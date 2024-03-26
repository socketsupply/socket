import { Environment } from './env.js'
import clients from './clients.js'

/**
 * A context given to `ExtendableEvent` interfaces and provided to
 * simplified service worker modules
 */
export class Context {
  #event = null

  /**
   * Context data. This may be a custom protocol handler scheme data
   * by default, if available.
   * @type {any?}
   */
  data = null

  /**
   * `Context` class constructor.
   * @param {import('./events.js').ExtendableEvent} event
   */
  constructor (event) {
    this.#event = event
  }

  /**
   * The `ExtendableEvent` for this `Context` instance.
   * @type {ExtendableEvent}
   */
  get event () {
    return this.#event
  }

  /**
   * An environment context object.
   * @type {object?}
   */
  get env () {
    return Environment.instance?.context ?? null
  }

  /**
   * Resets the current environment context.
   * @return {Promise<boolean>}
   */
  async resetEnvironment () {
    if (Environment.instance) {
      return await Environment.instance.reset()
    }

    return false
  }

  /**
   * Unused, but exists for cloudflare compat.
   * @ignore
   */
  passThroughOnException () {}

  /**
   * Tells the event dispatcher that work is ongoing.
   * It can also be used to detect whether that work was successful.
   * @param {Promise} promise
   */
  async waitUntil (promise) {
    return await this.event.waitUntil(promise)
  }

  /**
   * TODO
   */
  async handled () {
    return await this.event.handled
  }

  /**
   * Gets the client for this event context.
   * @return {Promise<import('./clients.js').Client>}
   */
  async client () {
    if (this.event.clientId) {
      return await clients.get(this.event.clientId)
    }

    return null
  }
}

export default {
  Context
}
