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
import ipc from '../ipc.js'
import gc from '../gc.js'
import { getGbnfGrammarForGbnfJsonSchema, validateObjectAgainstGbnfSchema } from './gbnf.js'

/**
 * Retrieves the completion text based on the provided input text and a list of full texts.
 *
 * @param {string | null} text - The input text for which completion is sought.
 * @param {string | string[]} fullText - A single full text string or an array of full text strings to search for completion.
 * @returns {string | null} - The completion text if found, or null if not found.
 */
function getTextCompletion (text, fullText) {
  if (text == null) return null

  const fullTexts = typeof fullText === 'string' ? [fullText] : fullText

  for (const fullText of fullTexts) {
    if (fullText.startsWith(text)) return fullText.slice(text.length)
  }

  return null
}

/**
 * Abstract class representing a Chat Prompt Wrapper.
 */
class ChatPromptWrapper {
  /**
   * The name of the wrapper.
   * @type {string}
   * @abstract
   */
  wrapperName = ''

  /**
   * Wraps a prompt with system prompt if it's not the first prompt.
   *
   * @param {string} prompt - The prompt to wrap.
   * @param {Object} options - Options for wrapping the prompt.
   * @param {string} options.systemPrompt - The system prompt to prepend.
   * @param {number} options.promptIndex - The index of the prompt.
   * @returns {string} - The wrapped prompt.
   */
  wrapPrompt (prompt, { systemPrompt, promptIndex }) {
    if (promptIndex === 0) {
      return systemPrompt + '\n' + prompt
    } else {
      return prompt
    }
  }

  /**
   * Retrieves an array of stop strings.
   *
   * @returns {string[]} - An array of stop strings.
   */
  getStopStrings () {
    return []
  }

  /**
   * Retrieves the default stop string.
   *
   * @returns {string} - The default stop string.
   * @throws {Error} - If the prompt wrapper has no stop strings defined.
   */
  getDefaultStopString () {
    const stopStrings = this.getStopStrings()
    const stopString = stopStrings[0]

    if (stopString == null || stopString.length === 0) {
      throw new Error(`Prompt wrapper "${this.wrapperName}" has no stop strings`)
    }

    return stopString
  }
}

/**
 * Class representing a General Chat Prompt Wrapper.
 */
class GeneralChatPromptWrapper extends ChatPromptWrapper {
  /**
   * The name of the wrapper.
   * @type {string}
   * @readonly
   */
  constructor ({ instructionName = 'Human', responseName = 'Assistant' } = {}) {
    super()

    /**
     * The name of the wrapper.
     * @type {string}
     * @readonly
     */
    this.wrapperName = 'General'

    /**
     * The name of the instruction.
     * @type {string}
     * @private
     */
    this._instructionName = instructionName

    /**
     * The name of the response.
     * @type {string}
     * @private
     */
    this._responseName = responseName
  }

  /**
     * Wraps a prompt with system prompt or response names.
     *
     * @param {string} prompt - The prompt to wrap.
     * @param {Object} options - Options for wrapping the prompt.
     * @param {string} options.systemPrompt - The system prompt to prepend.
     * @param {number} options.promptIndex - The index of the prompt.
     * @param {string | null} options.lastStopString - The last stop string.
     * @param {string | null} options.lastStopStringSuffix - The last stop string suffix.
     * @returns {string} - The wrapped prompt.
     */
  wrapPrompt (prompt, { systemPrompt, promptIndex, lastStopString, lastStopStringSuffix }) {
    if (promptIndex === 0) { return systemPrompt + `\n\n### ${this._instructionName}:\n` + prompt + `\n\n### ${this._responseName}:\n` }

    return this._getPromptPrefix(lastStopString, lastStopStringSuffix) + prompt + `\n\n### ${this._responseName}:\n`
  }

  /**
   * Retrieves an array of stop strings.
   *
   * @returns {string[]} - An array of stop strings.
   */
  getStopStrings () {
    return [
            `\n\n### ${this._instructionName}`,
            `### ${this._instructionName}`,
            `\n\n### ${this._responseName}`,
            `### ${this._responseName}`,
            '<end>'
    ]
  }

  /**
   * Retrieves the default stop string.
   *
   * @returns {string} - The default stop string.
   */
  getDefaultStopString () {
    return `\n\n### ${this._instructionName}`
  }

  /**
   * Gets the prompt prefix based on the last stop string and suffix.
   *
   * @private
   * @param {string | null} lastStopString - The last stop string.
   * @param {string | null} lastStopStringSuffix - The last stop string suffix.
   * @returns {string} - The prompt prefix.
   */
  _getPromptPrefix (lastStopString, lastStopStringSuffix) {
    const isEnd = lastStopString === '<end>'

    const a = isEnd
      ? lastStopStringSuffix
      : ((lastStopString ?? '') + (lastStopStringSuffix ?? ''))

    const b = [
      `\n\n### ${this._instructionName}:\n`,
      `### ${this._instructionName}:\n`
    ]

    return getTextCompletion(a, b) ?? `\n\n### ${this._instructionName}:\n`
  }
}

class Model {
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
      gpuLayers,
      vocabOnly,
      useMmap,
      useMlock
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

class Grammar {
  /**
   * > GBNF files are supported.
   * > More info here: [github:ggerganov/llama.cpp:grammars/README.md](
   * > https://github.com/ggerganov/llama.cpp/blob/f5fe98d11bdf9e7797bcfb05c0c3601ffc4b9d26/grammars/README.md)
   * @param {object} options
   * @param {string} options.grammar - GBNF grammar
   * @param {string[]} [options.stopStrings] - Consider any of these texts as EOS for the generated out.
   */
  constructor (args) {
    const {
      grammar,
      stopStrings = [],
      trimWhitespaceSuffix = false
    } = args

    const params = {
      text: grammar
    }

    const { data, err } = ipc.sendSync('llm.createGrammar', params, { cache: true })

    if (err) {
      throw new Error(err)
    }

    this._grammarId = data.grammarId
    this._stopStrings = stopStrings ?? []
    this._trimWhitespaceSuffix = trimWhitespaceSuffix
    this._grammarText = grammar
  }

  get grammar () {
    return this._grammarText
  }

  get stopStrings () {
    return this._stopStrings
  }

  get trimWhitespaceSuffix () {
    return this._trimWhitespaceSuffix
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

/**
 * Class representing a JsonSchemaGrammar.
 * @template {Readonly<GbnfJsonSchema>} T
 */
class JsonSchemaGrammar extends Grammar {
  /**
   * Creates an instance of JsonSchemaGrammar.
   * @param {T} schema - The Gbnf JSON schema to use for parsing.
   */
  constructor (schema) {
    const grammar = getGbnfGrammarForGbnfJsonSchema(schema)

    super({
      grammar,
      stopStrings: ['\n'.repeat(4)],
      trimWhitespaceSuffix: true
    })

    this._schema = schema
  }

  /**
   * Parses a JSON string and validates it against the specified Gbnf JSON schema.
   * @param {string} json - The JSON string to parse and validate.
   * @returns {GbnfJsonSchemaToType<T>} - The parsed and validated JSON object.
   */
  parse (json) {
    const parsedJson = JSON.parse(json)
    validateObjectAgainstGbnfSchema(parsedJson, this._schema)
    return parsedJson
  }
}

class Context {
  _contextId = null
  _model = null
  _prependBos = true
  _prependTokens = []

  /**
   * @typedef {Object} ContextOptions
   * @property {Model} model - The model to use.
   * @property {boolean} [prependBos] - Whether to prepend Beginning Of Sequence token.
   * @property {Grammar} [grammar] - This option is deprecated. Use the `grammar` option on `ChatSession`'s `prompt` function or the `grammarEvaluationState` option on `Context`'s `evaluate` function instead. This property is hidden from the documentation.
   * @property {(number|null)} [seed] - If null, a random seed will be used.
   * @property {number} [contextSize] - Text context size.
   * @property {number} [batchSize] - Prompt processing batch size.
   * @property {boolean} [logitsAll] - The llama_eval() call computes all logits, not just the last one.
   * @property {boolean} [embedding] - Embedding mode only.
   * @property {number} [threads] - Number of threads to use to evaluate tokens.
   */
  constructor (args) {
    const {
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

    if (data.token === -1) { return null }

    return data.token
  }

  async * eval (tokens, args) {
    const {
      temperature,
      topK,
      topP,
      grammarEvaluationState,
      repeatPenalty
    } = args

    let evalTokens = tokens

    if (this._prependTokens.length > 0) {
      const tokenArray = this._prependTokens.concat(Array.from(tokens))

      evalTokens = Uint32Array.from(tokenArray)
      this._prependTokens = []
    }

    if (evalTokens.length === 0) return

    while (true) {
      const param = {
        contextId,
        modelId,
        temperature,
        topK,
        topP,
        repeatPenalty: repeatPenalty?.penalty,
        repeatPenaltyTokens: repeatPenalty?.punishTokens instanceof Function
          ? repeatPenalty.punishTokens()
          : repeatPenalty?.punishTokens,
        repeatPenaltyPresencePenalty: repeatPenalty?.presencePenalty,
        repeatPenaltyFrequencyPenalty: repeatPenalty?.frequencyPenalty,
        grammarEvaluationState: grammarEvaluationState?._state
      }

      const { data: nextToken, err } = await ipc.send('llm.eval', params, { cache: true })
      if (err) throw new Erorr(err)

      // the assistant finished answering
      if (nextToken === this._ctx.tokenEos()) break

      yield nextToken

      // Create tokens for the next eval.
      evalTokens = Uint32Array.from([nextToken])
    }
  }
}

class GrammarEvaluationState {
  constructor (args) {
    const {
      grammar
    } = args

    const params = {
      grammarId: grammar._grammarId
    }

    const { data, err } = ipc.sendSync('llm.createEvaluator', params, { cache: true })

    if (err) {
      throw new Error(err)
    }

    this._evaluatorId = data.evaluatorId
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

export {
  getGbnfGrammarForGbnfJsonSchema,
  ChatPromptWrapper,
  Context,
  GeneralChatPromptWrapper,
  Grammar,
  GrammarEvaluationState,
  JsonSchemaGrammar,
  Model
}
