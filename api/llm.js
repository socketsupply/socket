/**
 * @module LLM
 *
 * This module provides an API for loading and working with large language models.
 *
 * Example usage:
 * ```js
 * import { Model, Context, Grammar } from 'socket:llm'
 * ```
 */
import ipc from './ipc.js'
import gc from './gc.js'

export class LlamaModel {
  _modelId = null

  /**
   * > options source:
   * > [github:ggerganov/llama.cpp/llama.h](
   * > https://github.com/ggerganov/llama.cpp/blob/b5ffb2849d23afe73647f68eec7b68187af09be6/llama.h#L102) (`struct llama_context_params`)
   * @param {object} options
   * @param {string} options.path - path to the model on the filesystem
   * @param {number | null} [options.seed] - If null, a random seed will be used
   * @param {number} [options.contextSize] - text context size
   * @param {number} [options.batchSize] - prompt processing batch size
   * @param {number} [options.gpuLayers] - number of layers to store in VRAM
   * @param {number} [options.threads] - number of threads to use to evaluate tokens
   * @param {number} [options.temperature] - Temperature is a hyperparameter that controls the randomness of the generated text.
   * It affects the probability distribution of the model's output tokens.
   * A higher temperature (e.g., 1.5) makes the output more random and creative,
   * while a lower temperature (e.g., 0.5) makes the output more focused, deterministic, and conservative.
   * The suggested temperature is 0.8, which provides a balance between randomness and determinism.
   * At the extreme, a temperature of 0 will always pick the most likely next token, leading to identical outputs in each run.
   *
   * Set to `0` to disable.
   * @param {number} [options.topK] - Limits the model to consider only the K most likely next tokens for sampling at each step of
   * sequence generation.
   * An integer number between `1` and the size of the vocabulary.
   * Set to `0` to disable (which uses the full vocabulary).
   *
   * Only relevant when `temperature` is set to a value greater than 0.
   * @param {number} [options.topP] - Dynamically selects the smallest set of tokens whose cumulative probability exceeds the threshold P,
   * and samples the next token only from this set.
   * A float number between `0` and `1`.
   * Set to `1` to disable.
   *
   * Only relevant when `temperature` is set to a value greater than `0`.
   * @param {boolean} [options.vocabOnly] - only load the vocabulary, no weights
   * @param {boolean} [options.useMmap] - use mmap if possible
   * @param {boolean} [options.useMlock] - force system to keep model in RAM
   */
  constructor (args) {
    const {
      path,
      seed = null,
      contextSize = 1024 * 4,
      batchSize,
      gpuLayers,
      threads = 6,
      temperature = 0,
      topK = 40,
      topP = 0.95,
      vocabOnly,
      useMmap,
      useMlock,
      embedding
    } = args

    const params = {
      path,
      gpuLayers,
      vocabOnly,
      useMmap,
      useMlock
    }

    const { data, err } = ipc.sendSync('llm.createModel', params, { cache: true })

    if (err) {
      throw new Error(err)
    }

    this._modelId = data.modelId
  }

  [gc.finalizer] () {
    return {
      args: [this._modelId],
      async handle (modelId) {
        return await ipc.send('llm.destroyModel', { modelId }, { cache: true })
      }
    }
  }
}


export class LlamaGrammar {
  constructor () {

  }

  [gc.finalizer] () {
    return {
      args: [this._grammarId],
      async handle (grammarId) {
        return await ipc.send('llm.destroyGrammar', { grammarId }, { cache: true })
      }
    }
  }
}

export class LlamaContext {
  _contextId = null
  _model = null
  _prependBos = true;
  _prependTokens = [];

  constructor (args) {
    const  {
      model,
      prependBos = true,
      grammar, // an instance of Grammar
      seed,
      contextSize,
      batchSize,
      logitsAll,
      embedding,
      threads
    } = args

    this._model = model
    this._chatGrammar = grammar
    this._prependBos = prependBos

    const params = {
      seed: seed != null ? Math.max(-1, seed) : undefined,
      contextSize,
      batchSize,
      logitsAll,
      embedding,
      threads
    }

    const { data, err } = ipc.sendSync('llm.createContext', params, { cache: true })

    if (err) {
      throw new Error(err)
    }

    this._contextId = data.contextId

    if (prependBos) {
      const { data, err } = ipc.sendSync('llm.tokenBos', { modelId: this._model._modelId }, { cache: true })
      if (err) throw new Error(err)

      this._prependTokens.unshift(data.token)
    }
  }

  [gc.finalizer] () {
    return {
      args: [this._contextId],
      async handle (contextId) {
        return await ipc.send('llm.destroyContext', { contextId }, { cache: true })
      }
    }
  }

  /**
   * @returns {Uint32Array} The BOS (Beginning Of Sequence) token.
   */
  encode (text) {
    if (text === '') return new Uint32Array()

    const { data, err } = ipc.sendSync('llm.encode', { modelId: this._contextId }, { cache: true })
    if (err) throw new Error(err)

    return Uint32Array.from(data)
  }

  /**
   * @returns {Token | null} The BOS (Beginning Of Sequence) token.
   */
  getBosToken () {
    const { data, err } = ipc.sendSync('llm.tokenBos', { modelId: this._model._modelId }, { cache: true })
    if (err) throw new Error(err)

    if (data.token === -1)
        return null;

    return data.token;
  }

  async *evaluate () {

  }
}

export class LlamaGrammarEvaluationState {
  constructor (args) {
    const {
      temperature,
      topK,
      topP
    } = args
  }

  [gc.finalizer] () {
    return {
      args: [this._evaluatorId],
      async handle (evaluatorId) {
        return await ipc.send('llm.destroyEvaluator', { evaluatorId }, { cache: true })
      }
    }
  }
}
