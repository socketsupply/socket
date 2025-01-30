import { Context, Model } from './llm.js'
import { MessageEvent } from '../events.js'
import { Conduit } from '../internal/conduit.js'
import { Buffer } from '../buffer.js'
import { rand64 } from '../crypto.js'
import process from '../process.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

/**
 * @typedef {import('./llm.js').ModelOptions} ModelOptions
 * @typedef {import('./llm.js').ModelLoadOptions} ModelLoadOptions
 * @typedef {import('./llm.js').ContextOptions} ContextOptions
 */

export class ChatMessageEvent extends MessageEvent {
  #finished = false
  /**
   * @param {string} type
   * @param {MessageEventInit & { finished?: boolean }} options
   */
  constructor (type, options) {
    super(type, options)
    this.#finished = options?.finished ?? false
  }

  get finished () {
    return this.#finished
  }
}

/**
 * @typedef {{
 *   id: string,
 *   role: string,
 *   content: string
 * }} MessageOptions
 */

export class Message {
  #id
  #role
  #content

  constructor (options) {
    this.#id = options.id
    this.#role = options.role
    this.#content = options.content
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
  get role () {
    return this.#role
  }

  /**
   * @type {string}
   */
  get content () {
    return this.#content
  }
}

/**
 * @typedef {{
 *   id?: string,
 *   antiprompts?: Set<string>|string[]
 * }} SessionOptions
 */

export class Session extends EventTarget {
  #id
  /**
   * @type {Context}
   */
  #context
  /**
   * @type {Message[]}
   */
  #messages = []
  /**
   * @type {Conduit}
   */
  #conduit
  #prompt = ''
  #loaded = false
  #started = false
  #generating = false
  #buffer = []
  /**
   * @type {Set<string>}
   */
  #antiprompts = new Set()

  /**
   * @param {Context} context
   * @param {SessionOptions=} [options]
   */
  constructor (context, options = null) {
    super()
    this.#id = options?.id ?? rand64()
    this.#context = context
    this.#conduit = new Conduit({ id: this.#id })

    if (typeof options?.prompt === 'string') {
      this.#prompt = options.prompt
    }

    if (
      Array.isArray(options?.antiprompts) ||
      options?.antiprompts instanceof Set
    ) {
      for (const antiprompt of options.antiprompts) {
        this.#antiprompts.add(antiprompt)
      }
    }
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
  get prompt () {
    return this.#prompt
  }

  /**
   * @type {Context}
   */
  get context () {
    return this.#context
  }

  /**
   * @type {Conduit}
   */
  get conduit () {
    return this.#conduit
  }

  /**
   * @type {boolean}
   */
  get started () {
    return this.#started
  }

  /**
   * @type {boolean}
   */
  get loaded () {
    return this.#loaded
  }

  /**
   * @type {boolean}
   */
  get generating () {
    return this.#generating
  }

  /**
   * @type {Message[]}
   */
  get messages () {
    return this.#messages
  }

  /**
   * @type {Set<string>}
   */
  get antiprompts () {
    return this.#antiprompts
  }

  /**
   * @param {Model} model
   * @param {(ModelLoadOptions & ContextOptions)=} [options]
   * @return {Promise}
   */
  async load (model, options) {
    if (this.#loaded) {
      return
    }

    await this.#context.load(model, options)
    if (!this.#context.loaded) {
      return
    }

    this.#loaded = true
  }

  /**
   * @return {Promise}
   */
  async start () {
    if (!this.#loaded) {
      return
    }

    this.#conduit.receive((_, decoded) => {
      if (!decoded || !decoded.options) return
      if (
        decoded.options.source === 'ai.chat.session.message' ||
        decoded.options.source === 'ai.chat.session.generate'
      ) {
        const data = Buffer.from(decoded.payload)
        const finished = (
          decoded.options.eog ||
          decoded.options.complete ||
          decoded.options.finished
        )

        this.#buffer.push(data)
        // @ts-ignore
        this.dispatchEvent(new ChatMessageEvent('message', {
          finished,
          data
        }))

        if (finished) {
          const buffer = Buffer.concat(this.#buffer.splice(0, this.#buffer.length))
          if (decoded.options.source === 'ai.chat.session.message') {
            this.#messages.push(new Message({
              id: decoded.options.id,
              role: 'assistant',
              content: buffer.toString()
            }))
          }
        }
      }
    })

    this.#started = true
    gc.ref(this)
  }

  /**
   * @param {GenerateOptions=} [options]
   * @return {Promise<object>}
   */
  async generate (options) {
    if (!this.started) {
      await this.start()
    }

    const prompt = Buffer.concat([
      Buffer.from(this.#prompt),
      Buffer.from(options?.prompt)
    ])
    const result = await ipc.write('ai.chat.session.generate', {
      id: this.#id,
      antiprompts: Array.from(new Set(Array
        .from(this.#antiprompts)
        .concat(options?.antiprompts || [])
        .filter(Boolean)
        .filter((value) => typeof value === 'string')
      )).join('\x01')
    }, prompt)

    if (result.err) {
      throw result.err
    }

    return result.data
  }

  async message (options) {
    if (!this.started) {
      await this.start()
    }

    const prompt = Buffer.concat([
      Buffer.from(this.#prompt),
      Buffer.from(options?.prompt)
    ])
    const result = await ipc.write('ai.chat.session.message', {
      id: this.#id,
      antiprompts: Array.from(new Set(Array
        .from(this.#antiprompts)
        .concat(options?.antiprompts || [])
        .filter(Boolean)
        .filter((value) => typeof value === 'string')
      )).join('\x01')
    }, prompt)

    if (result.err) {
      throw result.err
    }

    this.#messages.push(new Message({
      id: result.data.id,
      role: 'user',
      content: options.prompt
    }))

    return result.data
  }

  /**
   * @ignore
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, this.#conduit, options],
      async handle (id, conduit) {
        if (process.env.DEBUG) {
          console.warn('ai.chat: Closing session on garbage collection')
        }

        await conduit.close()
        await ipc.request('ai.llm.context.destroy', { id }, options)
      }
    }
  }
}

/**
 * @typedef {SessionOptions & {
 *   model: string | (ModelOptions & ModelLoadOptions),
 *   context?: ContextOptions
 * }} ChatOptions
 */

export class Chat extends Session {
  #id
  #model
  #options
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
   * @param {ChatOptions} options
   */
  constructor (options) {
    const id = String(rand64())

    super(
      new Context({ ...options?.context, id }),
      { ...options, id }
    )

    this.#id = id
    this.#options = options

    if (typeof options?.model === 'string') {
      this.#model = new Model({ name: options.model })
    } else if (options.model && typeof options?.model === 'object') {
      this.#model = new Model(options.model)
    }
  }

  /**
   * @type {string}
   */
  get id () {
    return this.#id
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
   * @return {Promise}
   */
  async load () {
    if (this.#loaded) {
      return
    }

    await this.#model.load(this.#options.model)
    await super.load(this.#model, this.#options.context)
    this.#loaded = super.loaded
    this.#ready.resolve()
  }
}

export default {
  Message,
  Session,
  Chat
}
