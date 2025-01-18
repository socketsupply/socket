import { rand64 } from '../crypto.js'
import ipc from '../ipc.js'

/**
 * @typedef {{
 *   name: string,
 * }} ModelOptions
 */

/**
 * @typedef {{
 *   directory?: string,
 *   gpuLayerCount?: number
 * }} ModelLoadOptions
 */

export class Model {
  #id
  #name
  #loaded = false
  /**
   * @type {{
   *   promise: Promise,
   *   resolve: function(any):void,
   *   reject: function(Error):void
   * }}
   */
  #ready = (
    // @ts-ignore
    Promise.withResolvers()
  )

  /**
   * @param {ModelOptions} options
   */
  constructor (options) {
    this.#name = options.name
  }

  /**
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * @type {Promise}
   */
  get ready () {
    return this.#ready.promise
  }

  get loaded () {
    return this.#loaded
  }

  /**
   * @param {ModelLoadOptions=} [options]
   */
  async load (options = null) {
    if (this.#loaded) {
      return this.#ready.promise
    }

    const result = await ipc.request('ai.llm.model.load', {
      gpuLayerCount: options?.gpuLayerCount || '',
      directory: options?.directory || '',
      name: this.#name
    })

    if (result.err) {
      throw result.err
    }

    this.#id = result.data.id

    this.#loaded = true
    this.#ready.resolve()
  }
}

/**
 * @typedef {{
 *   size?: number,
 *   minP?: number,
 *   temp?: number,
 *   topK?: number,
 *   id?: string
 * }} ContextOptions
 */

/**
 * @typedef {{
 *   id: string,
 *   size: number,
 *   used: number
 * }} ContextStats
 */

export class Context {
  #id = String(rand64())
  #size = 0
  /**
   * @type {Model}
   */
  #model
  #loaded = false
  /**
   * @type {ContextOptions}
   */
  #options = /** @type {ContextOptions} */ ({
    size: 2048,
    minP: 0.05,
    temp: 0.8
  })

  /**
   * @type {{
   *   promise: Promise,
   *   resolve: function(any):void,
   *   reject: function(Error):void
   * }}
   */
  #ready = (
    // @ts-ignore
    Promise.withResolvers()
  )

  /**
   * @param {ContextOptions=} [options]
   */
  constructor (options = null) {
    this.#id = options?.id || this.#id
    this.#options.size = Number.isFinite(options?.size)
      ? options.size
      : this.#options.size

    this.#options.minP = Number.isFinite(options?.minP)
      ? options.minP
      : this.#options.minP

    this.#options.temp = Number.isFinite(options?.temp)
      ? options.temp
      : this.#options.temp

    if (Number.isFinite(options?.topK)) {
      // @ts-ignore
      this.#options.topK = options.topK
    }
  }

  /**
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * @type {number}
   */
  get size () {
    return this.#size
  }

  /**
   * @type {boolean}
   */
  get loaded () {
    return this.#loaded
  }

  /**
   * @type {Model}
   */
  get model () {
    return this.#model
  }

  /**
   * @type {Promise}
   */
  get ready () {
    return this.#ready.promise
  }

  /**
   * @type {ContextOptions}
   */
  get options () {
    return { ...this.#options }
  }

  /**
   * @param {Model} model
   * @param {ContextOptions=} [options]
   * @return {Promise}
   */
  async load (model, options) {
    if (this.loaded) {
      return
    }

    await model.load()

    if (!model.loaded) {
      throw new TypeError(`Failed to load model: "${model.name}"`)
    }

    const result = await ipc.request('ai.llm.context.create', {
      ...this.#options,
      ...options,
      model: model.id,
      id: this.#id
    })

    if (result.err) {
      throw result.err
    }

    this.#id = result.data.id
    this.#model = model
    this.#loaded = true
    this.#ready.resolve()
  }

  /**
   * @return {Promise<ContextStats>}
   */
  async stats () {
    if (!this.loaded) {
      throw new TypeError('Context is not loaded')
    }

    const result = await ipc.request('ai.llm.context.stats', {
      id: this.id
    })

    if (result.err) {
      throw result.err
    }

    return /** @type {ContextStats} */ (result.data)
  }
}

export default {
  Model,
  Context
}
