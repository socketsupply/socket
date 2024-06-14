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
import { EventEmitter } from './events.js'
import { rand64 } from './crypto.js'
import process from './process.js'
import ipc from './ipc.js'
import gc from './gc.js'

import * as exports from './ai.js'

/**
 * A class to interact with large language models (using llama.cpp)
 */
export class LLM extends EventEmitter {
  /**
   * Constructs an LLM instance. Each parameter is designed to configure and control
   * the behavior of the underlying large language model provided by llama.cpp.
   * @param {Object} options - Configuration options for the LLM instance.
   * @param {string} options.path - The file path to the model in .gguf format. This model file contains
   *                                the weights and configuration necessary for initializing the language model.
   * @param {string} options.prompt - The initial input text to the model, setting the context or query
   *                                  for generating responses. The model uses this as a starting point for text generation.
   * @param {string} [options.id] - An optional unique identifier for this specific instance of the model,
   *                                useful for tracking or referencing the model in multi-model setups.
   * @param {number} [options.n_ctx=1024] - Specifies the maximum number of tokens that the model can consider
   *                                        for a single query. This is crucial for managing memory and computational
   *                                        efficiency. Exceeding the model's configuration may lead to errors or truncated outputs.
   * @param {number} [options.n_threads=8] - The number of threads allocated for the model's computation,
   *                                         affecting performance and speed of response generation.
   * @param {number} [options.temp=1.1] - Sampling temperature controls the randomness of predictions.
   *                                      Higher values increase diversity, potentially at the cost of coherence.
   * @param {number} [options.max_tokens=512] - The upper limit on the number of tokens that the model can generate
   *                                            in response to a single prompt. This prevents runaway generations.
   * @param {number} [options.n_gpu_layers=32] - The number of GPU layers dedicated to the model processing.
   *                                             More layers can increase accuracy and complexity of the outputs.
   * @param {number} [options.n_keep=0] - Determines how many of the top generated responses are retained after
   *                                      the initial generation phase. Useful for models that generate multiple outputs.
   * @param {number} [options.n_batch=0] - The size of processing batches. Larger batch sizes can reduce
   *                                       the time per token generation by parallelizing computations.
   * @param {number} [options.n_predict=0] - Specifies how many forward predictions the model should make
   *                                         from the current state. This can pre-generate responses or calculate probabilities.
   * @param {number} [options.grp_attn_n=0] - Group attention parameter 'N' modifies how attention mechanisms
   *                                          within the model are grouped and interact, affecting the model’s focus and accuracy.
   * @param {number} [options.grp_attn_w=0] - Group attention parameter 'W' adjusts the width of each attention group,
   *                                          influencing the breadth of context considered by each attention group.
   * @param {number} [options.seed=0] - A seed for the random number generator used in the model. Setting this ensures
   *                                    consistent results in model outputs, important for reproducibility in experiments.
   * @param {number} [options.top_k=0] - Limits the model's output choices to the top 'k' most probable next words,
   *                                     reducing the risk of less likely, potentially nonsensical outputs.
   * @param {number} [options.tok_p=0.0] - Top-p (nucleus) sampling threshold, filtering the token selection pool
   *                                      to only those whose cumulative probability exceeds this value, enhancing output relevance.
   * @param {number} [options.min_p=0.0] - Sets a minimum probability filter for token generation, ensuring
   *                                      that generated tokens have at least this likelihood of being relevant or coherent.
   * @param {number} [options.tfs_z=0.0] - Temperature factor scale for zero-shot learning scenarios, adjusting how
   *                                      the model weights novel or unseen prompts during generation.
   * @throws {Error} Throws an error if the model path is not provided, as the model cannot initialize without it.
   */

  constructor (options = null) {
    super()

    options = { ...options }
    if (!options.path) {
      throw new Error('expected a path to a valid model (.gguf)')
    }

    this.path = options.path
    this.prompt = options.prompt
    this.id = options.id || rand64()

    const opts = {
      id: this.id,
      path: this.path,
      prompt: this.prompt,
      // @ts-ignore
      antiprompt: options.antiprompt,
      // @ts-ignore
      conversation: options.conversation === true,
      // @ts-ignore
      chatml: options.chatml === true,
      // @ts-ignore
      instruct: options.instruct === true,
      n_ctx: options.n_ctx || 1024, // simplified, assuming default value of 1024 if not specified
      n_threads: options.n_threads || 8,
      temp: options.temp || 1.1, // assuming `temp` should be a number, not a string
      max_tokens: options.max_tokens || 512,
      n_gpu_layers: options.n_gpu_layers || 32,
      n_keep: options.n_keep || 0,
      n_batch: options.n_batch || 0,
      n_predict: options.n_predict || 0,
      grp_attn_n: options.grp_attn_n || 0,
      grp_attn_w: options.grp_attn_w || 0,
      seed: options.seed || 0,
      top_k: options.top_k || 0,
      tok_p: options.tok_p || 0.0,
      min_p: options.min_p || 0.0,
      tfs_z: options.tfs_z || 0.0
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

        this.emit('data', decodeURIComponent(data.token))
      }
    })

    ipc.request('ai.llm.create', opts)
      .then((result) => {
        if (result.err) {
          this.emit('error', result.err)
        }
      }, (err) => {
        this.emit('error', err)
      })
  }

  /**
   * Tell the LLM to stop after the next token.
   * @returns {Promise<void>} A promise that resolves when the LLM stops.
   */
  async stop () {
    return await ipc.request('ai.llm.stop', { id: this.id })
  }

  /**
   * @ignore
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, options],
      async handle (id) {
        if (process.env.DEBUG) {
          console.warn('Closing LLM on garbage collection')
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
    return await ipc.request('ai.llm.chat', { id: this.id, message })
  }
}

export default exports
