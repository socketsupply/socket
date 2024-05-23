// @ts-check
/**
 * @module AI
 *
 * Provides high level classes for common AI tasks
 *
 * Example usage:
 * ```js
 * import { LLM } from 'socket:ai'
 * ```
 */
import ipc from './ipc.js'
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
   * @param {Object} options - The options for initializing the LLM.
   * @param {string} options.path - The path to a valid model (.gguf).
   * @param {string} options.prompt - The query that guides the model to generate a relevant and coherent responses.
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

    globalThis.addEventListener('data', ({ detail }) => {
      const { err, data, source } = detail.params

      if (err && BigInt(err.id) === this.id) {
        return this.emit('error', err)
      }

      if (!data || BigInt(data.id) !== this.id) return

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
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @param {Object} options - The options for finalizer.
   * @returns {gc.Finalizer} The finalizer object.
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
