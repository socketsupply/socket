/* global ErrorEvent, EventTarget */
import database from './database.js'

/**
 * @typedef {{
 *   scope: string
 * }} EnvironmentOptions
 */

/**
 * Awaits a promise forwarding errors to the `Environment` instance.
 * @ignore
 * @param {Environment} env
 * @param {Promise} promise
 */
async function forward (env, promise) {
  try {
    return await promise
  } catch (error) {
    env.dispatchEvent(new ErrorEvent('error', { error }))
  }
}

/**
 * An environment context object with persistence and durability
 * for service worker environments.
 */
export class Environment extends EventTarget {
  /**
   * Opens an environment for a particular scope.
   * @param {EnvironmentOptions} options
   * @return {Environment}
   */
  static async open (options) {
    const environment = new this(options)
    await environment.open()
    return environment
  }

  /**
   * The current `Environment` instance
   * @type {Environment?}
   */
  static instance = null

  #database = null
  #context = {}
  #proxy = null
  #scope = null

  /**
   * `Environment` class constructor
   * @ignore
   * @param {EnvironmentOptions} options
   */
  constructor (options) {
    super()

    Environment.instance = this

    this.#scope = options.scope
    this.#proxy = new Proxy(this.#context, {
      get: (target, property) => {
        return target[property] ?? undefined
      },

      set: (target, property, value) => {
        target[property] = value
        if (this.database && this.database.opened) {
          forward(this.database.put(property, value))
        }
        return true
      },

      deleteProperty: (target, property) => {
        if (this.database && this.database.opened) {
          forward(this.database.delete(property))
        }
        return Reflect.deleteProperty(target, property)
      },

      has: (target, property) => {
        return Reflect.has(target, property)
      }
    })
  }

  /**
   * A reference to the currently opened environment database.
   * @type {import('./database.js').Database}
   */
  get database () {
    return this.#database
  }

  /**
   * A proxied object for reading and writing environment state.
   * Values written to this object must be cloneable with respect to the
   * structured clone algorithm.
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm}
   * @type {Proxy<object>}
   */
  get context () {
    return this.#proxy
  }

  /**
   * The current environment name. This value is also used as the
   * internal database name.
   * @type {string}
   */
  get name () {
    return `socket.runtime.serviceWorker.env(${this.#scope})`
  }

  /**
   * Resets the current environment to an empty state.
   */
  async reset () {
    await this.close()
    await database.drop(this.name)
    await this.open()
  }

  /**
   * Opens the environment.
   * @ignore
   */
  async open () {
    this.#database = await database.open(this.name)
    const entries = await this.#database.entries()

    for (const [key, value] of entries) {
      this.#context[key] = value
    }
  }

  /**
   * Closes the environment database, purging existing state.
   * @ignore
   */
  async close () {
    await this.#database.close()
    for (const key in this.#context) {
      Reflect.deleteProperty(this.#context, key)
    }
  }
}

/**
 * Opens an environment for a particular scope.
 * @param {EnvironmentOptions} options
 * @return {Promise<Environment>}
 */
export async function open (options) {
  return await Environment.open(options)
}

/**
 * Closes an active `Environment` instance, dropping the global
 * instance reference.
 * @return {Promise<boolean>}
 */
export async function close () {
  if (Environment.instance) {
    const instance = Environment.instance
    Environment.instance = null
    await instance.close()
    return true
  }

  return false
}

/**
 * Resets an active `Environment` instance
 * @return {Promise<boolean>}
 */
export async function reset () {
  if (Environment.instance) {
    const instance = Environment.instance
    await instance.reset()
    return true
  }

  return false
}

export default {
  Environment,
  close,
  reset,
  open
}
