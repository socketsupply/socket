// @ts-check
/**
 * @module AI
 *
 * Provides high level classes for common AI tasks.
 *
 * If you download a model like `mistral-7b-openorca.Q4_0.gguf` from Hugging
 * Face, you can construct in JavaScript with a prompt. Prompt syntax isn't
 * concrete like programming syntax, so you'll usually want to know what the
 * author has to say about prompting, for example this might be worth reading...
 *
 * https://docs.mistral.ai/guides/prompting_capabilities
 *
 * Example usage:
 *
 * ```js
 * import { LLM } from 'socket:ai'
 *
 * const llm = new LLM({
 *   path: 'model.gguf',
 *   prompt: '...' // insert your prompt here.
 * })
 *
 * llm.on('end', () => {
 *   // end of the token stream.
 * })
 *
 * llm.on('data', data => {
 *   // a new token has arrived in the token stream.
 * })
 * ```
 */
import ipc from './ipc.js'
import process from './process.js'
import gc from './gc.js'
import { EventEmitter } from './events.js'
import { rand64 } from './crypto.js'
import * as exports from './ai.js'

/**
 * A class to interact with large language models (using llama.cpp)
 * @extends EventEmitter
 */
export class LLM extends EventEmitter {
  /**
   * Constructs an LLM instance.
   * @param {Object} [options] - The options for initializing the LLM.
   * @param {string} [options.path] - The path to a valid model (.gguf).
   * @param {string} [options.prompt] - The query that guides the model to generate a relevant and coherent responses.
   * @param {string} [options.id] - The optional ID for the LLM instance.
   * @throws {Error} If the model path is not provided.
   */
  constructor (options = {}) {
    super()

    if (!options.path) {
      throw new Error('expected a path to a valid model (.gguf)')
    }

    this.path = options.path
    this.prompt = options.prompt
    this.id = options.id || rand64()

    const opts = {
      id: this.id,
      prompt: this.prompt,
      path: this.path
    }

    globalThis.addEventListener('data', event => {
      // @ts-ignore
      const detail = event.detail
      const { err, data, source } = detail.params

      if (err && BigInt(err.id) === this.id) {
        return this.emit('error', err)
      }

      if (!data || BigInt(data.id) !== this.id) return

      if (source === 'ai.llm.log') {
        this.emit('log', data.message)
        return
      }

      if (source === 'ai.llm.chat') {
        if (data.complete) {
          return this.emit('end')
        }

        this.emit('data', data.token)
      }
    })

    const result = ipc.sendSync('ai.llm.create', opts)

    if (result.err) {
      throw result.err
    }
  }

  /**
   * Tell the LLM to stop after the next token.
   * @returns {Promise<void>} A promise that resolves when the LLM stops.
   */
  async stop () {
    return ipc.request('ai.llm.stop', { id: this.id })
  }

  /**
   * @ignore
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, options],
      async handle (id) {
        if (process.env.DEBUG) {
          console.warn('Closing Socket on garbage collection')
        }

        await ipc.request('ai.llm.destroy', { id }, options)
      }
    }
  }

  /**
   * Send a message to the chat.
   * @param {string} message - The message to send to the chat.
   * @returns {Promise<any>} A promise that resolves with the response from the chat.
   */
  async chat (message) {
    return ipc.request('ai.llm.chat', { id: this.id, message })
  }
}

export default exports
