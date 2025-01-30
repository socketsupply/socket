import { rand64 } from '../crypto.js'
import ipc from '../ipc.js'

/**
 * @typedef {{ name: string, }} ModelOptions
 * @typedef {{ directory?: string, gpuLayerCount?: number }} ModelLoadOptions
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

  /**
   * `true` if the model is loaded, otherwise `false`.
   * @type {boolean}
   */
  get loaded () {
    return this.#loaded
  }

  /**
   * Loads the model it not already loaded.
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

  toJSON () {
    return {
      name: this.#name
    }
  }
}

/**
 * @typedef {{ name: string, }} LoRAOptions
 * @typedef {{ directory?: string, gpuLayerCount?: number }} LoRALoadOptions
 * @typedef {{ scale?: number }} LoraAttachOptions
 */

export class LoRA {
  #id
  #name
  #loaded = false
  #model
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
   * @param {Model} model
   * @param {LoRAOptions} options
   */
  constructor (model, options) {
    this.#model = model
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
   * @param {LoRALoadOptions=} [options]
   */
  async load (options = null) {
    if (this.#loaded) {
      return this.#ready.promise
    }

    const result = await ipc.request('ai.llm.lora.load', {
      directory: options?.directory || '',
      model: this.#model.id,
      name: this.#name
    })

    if (result.err) {
      throw result.err
    }

    this.#id = result.data.id

    this.#loaded = true
    this.#ready.resolve()
  }

  /**
   * Attach a LoRA to a context.
   * @param {Context} context
   * @param {LoraAttachOptions=} [options]
   * @return {Promise}
   */
  async attach (context, options) {
    if (!this.#loaded) {
      await this.load(options)
    }

    const result = await ipc.request('ai.llm.lora.attach', {
      context: context.id,
      scale: options?.scale || '',
      id: this.#id
    })

    if (result.err) {
      throw result.err
    }

    return result.data
  }

  /**
   * @param {Context} context
   * @return {Promise}
   */
  async detach (context) {
    if (!this.#loaded) {
      throw new TypeError(`LoRA "${this.#name}" is not loaded`)
    }

    const result = await ipc.request('ai.llm.lora.detach', {
      context: context.id,
      id: this.#id
    })

    if (result.err) {
      throw result.err
    }

    return result.data
  }

  toJSON () {
    return {
      name: this.#name,
      model: this.#model.toJSON()
    }
  }
}

/**
 * @typedef {
 *   context: Context,
 *   model: Model,
 *   lora: LoRA
 * {}} LoRAAttachmentOptions
 */

export class LoRAAttachment {
  #context
  #model
  #lora

  /**
   * @param {LoRAAttachmentOptions} options
   */
  constructor (options) {
    this.#context = options.context
    this.#model = options.model
    this.#lora = options.lora
  }

  /**
   * @type {Context}
   */
  get context () {
    return this.#context
  }

  /**
   * @type {Model}
   */
  get model () {
    return this.#model
  }

  /**
   * @type {LoRA}
   */
  get lora () {
    return this.#lora
  }

  toJSON () {
    return {
      context: this.#context.toJSON(),
      model: this.#model.toJSON(),
      lora: this.#lora.toJSON()
    }
  }
}

/**
 * @typedef {{
 *   size?: number,
 *   minP?: number,
 *   temp?: number,
 *   topK?: number,
 *   topP?: number,
 *   id?: string
 * }} ContextOptions
 *
 * @typedef {{
 *   id: string,
 *   size: number,
 *   used: number
 * }} ContextStats
 */

export class Context {
  #id = String(rand64())
  #size = 0
  #loaded = false
  /**
   * @type {Model}
   */
  #model
  /**
   * @type {Map<string, LoRAAttachment>}
   */
  #attachments = new Map()
  /**
   * @type {ContextOptions}
   */
  #options = /** @type {ContextOptions} */ ({
    size: 8 * 1024,
    temp: 0.80,
    topP: 0.95,
    minP: 0.05,
    topK: 40
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
      : this.#options.size || ''

    this.#options.minP = Number.isFinite(options?.minP)
      ? options.minP
      : this.#options.minP || 0

    this.#options.temp = Number.isFinite(options?.temp)
      ? options.temp
      : this.#options.temp || 0

    if (Number.isFinite(options?.topK)) {
      // @ts-ignore
      this.#options.topK = options.topK || 0
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
   * @type {LoRAAttachment[]}
   */
  get attachments () {
    return Array.from(this.#attachments.values())
  }

  /**
   * @type {LoRA[]}
   */
  get adapters () {
    return this.attachments.map((a) => a.lora)
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

  toJSON () {
    return {
      id: this.#id,
      size: this.#size,
      model: this.#model.toJSON()
    }
  }
}

export default {
  Model,
  LoRA,
  LoRAAttachment,
  Context
}
