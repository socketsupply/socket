import { defaultChatSystemPrompt } from '../config.js'
import { Model, GrammarEvaluationState } from '../index.js'

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

/**
 * ChatMLChatPromptWrapper is a class that extends ChatPromptWrapper and provides a custom implementation
 * for wrapping prompts and defining stop strings.
 */
class ChatMLChatPromptWrapper extends ChatPromptWrapper {
  /**
     * Creates an instance of ChatMLChatPromptWrapper.
     */
  constructor () {
    super()
    /**
         * The name of the ChatML wrapper.
         * @type {string}
         */
    this.wrapperName = 'ChatML'
  }

  /**
     * Overrides the wrapPrompt method to customize how prompts are wrapped.
     * @param {string} prompt - The user's input prompt.
     * @param {object} options - Options for wrapping the prompt.
     * @param {string} options.systemPrompt - The system's prompt.
     * @param {number} options.promptIndex - The index of the prompt.
     * @param {string|null} options.lastStopString - The last stop string.
     * @param {string|null} options.lastStopStringSuffix - The suffix of the last stop string.
     * @returns {string} The wrapped prompt.
     */
  wrapPrompt (prompt, { systemPrompt, promptIndex, lastStopString, lastStopStringSuffix }) {
    const previousCompletionEnd = (lastStopString ?? '') + (lastStopStringSuffix ?? '')

    if (promptIndex === 0 && systemPrompt !== '') {
      return (getTextCompletion(previousCompletionEnd, 'system\n') ?? 'system\n') +
                systemPrompt + '\nuser\n' + prompt + '\nassistant\n'
    } else {
      return (getTextCompletion(previousCompletionEnd, '\nuser\n') ?? '\nuser\n') +
                prompt + '\nassistant\n'
    }
  }

  /**
     * Overrides the getStopStrings method to provide custom stop strings.
     * @returns {string[]} An array of stop strings.
     */
  getStopStrings () {
    return ['']
  }

  /**
     * Overrides the getDefaultStopString method to provide a default stop string.
     * @returns {string} The default stop string.
     */
  getDefaultStopString () {
    return ''
  }
}

/**
 * Gets a chat wrapper class based on the provided beginning-of-sequence (BOS) string.
 * @param {string | undefined | null} bos - The beginning-of-sequence (BOS) string.
 * @returns {ChatPromptWrapper | null} The corresponding chat wrapper class or null if not found.
 */
function getChatWrapperByBos (bos) {
  if (bos === '' || bos == null) { return null }

  if ('<s>[INST] <<SYS>>\n'.startsWith(bos)) {
    return ChatPromptWrapper
  } else if ('system\n'.startsWith(bos)) {
    return ChatMLChatPromptWrapper
  }

  return null
}

/**
 * A map to store locks and associated promises.
 * @type {Map<any, Map<string, Promise<any>>>}
 */
const locks = new Map()

/**
 * Executes a callback function with a lock to ensure exclusive access.
 * @template ReturnType
 * @param {any} scope - The scope or identifier for the lock.
 * @param {string} key - The key for the lock.
 * @param {() => Promise<ReturnType>} callback - The asynchronous callback function to execute.
 * @returns {Promise<ReturnType>} A promise that resolves to the result of the callback.
 */
async function withLock (scope, key, callback) {
  while (locks.get(scope)?.has(key)) {
    await locks.get(scope)?.get(key)
  }

  const promise = callback()

  if (!locks.has(scope)) { locks.set(scope, new Map()) }

  locks.get(scope).set(key, promise)

  try {
    return await promise
  } finally {
    locks.get(scope)?.delete(key)

    if (locks.get(scope)?.size === 0) { locks.delete(scope) }
  }
}

/**
 * Represents an unknown Unicode character.
 * @type {string}
 */
const UNKNOWN_UNICODE_CHAR = '\ufffd'

/**
 * Options for configuring a ChatSession.
 * @typedef {Object} ChatSessionOptions
 * @property {Context} context - The context for the chat session.
 * @property {boolean} [printLLamaSystemInfo=false] - Whether to print  system information.
 * @property {ChatPromptWrapper|"auto"} [promptWrapper="auto"] - The chat prompt wrapper to use.
 * @property {string} [systemPrompt=""] - The system prompt for the chat session.
 * @property {readonly ConversationInteraction[]} [conversationHistory=[]] - Conversation history to load into the context.
 */

/**
 * Options for configuring a LLamaChatPrompt.
 * @typedef {Object} LLamaChatPromptOptions
 * @property {(tokens: Token[]) => void} [onToken] - A callback function to handle tokens.
 * @property {AbortSignal} [signal] - An optional abort signal.
 * @property {number} [maxTokens] - The maximum number of tokens to generate.
 * @property {number} [temperature] - The temperature hyperparameter for controlling randomness.
 * @property {number} [topK] - Limits the model to consider only the K most likely next tokens.
 * @property {number} [topP] - Dynamically selects the smallest set of tokens whose cumulative probability exceeds a threshold.
 * @property {Grammar} [grammar] - The grammar for the chat prompt.
 * @property {boolean} [trimWhitespaceSuffix=false] - Whether to trim whitespace from the end of generated text.
 * @property {false|ChatSessionRepeatPenalty} [repeatPenalty=false] - Repeat penalty options.
 */

/**
 * Options for configuring repeat penalties in a chat session.
 * @typedef {Object} ChatSessionRepeatPenalty
 * @property {number} [lastTokens=64] - Number of recent tokens generated by the model to apply penalties to repetition of.
 * @property {(tokens: Token[]) => Token[]} [punishTokensFilter] - A filter function for punishing specific tokens.
 * @property {boolean} [penalizeNewLine=true] - Whether to penalize new line tokens.
 * @property {number} [penalty=1.1] - The relative amount to lower the probability of the tokens.
 * @property {number} [frequencyPenalty=0] - Penalize tokens based on frequency.
 * @property {number} [presencePenalty=0] - Lower the probability of all tokens in the `punishTokens` array.
 */

/**
 * Represents a  chat session.
 */
export class ChatSession {
  /**
     * Creates a new ChatSession instance.
     * @param {ChatSessionOptions} options - The options for the chat session.
     */
  constructor ({
    context,
    printLLamaSystemInfo = false,
    promptWrapper = 'auto',
    systemPrompt = '',
    conversationHistory
  }) {
    this._ctx = context
    this._printLLamaSystemInfo = printLLamaSystemInfo
    this._systemPrompt = systemPrompt
    this._conversationHistoryToLoad = (conversationHistory != null && conversationHistory.length > 0)
      ? conversationHistory
      : null

    if (promptWrapper === 'auto') {
      const ChatWrapper = getChatWrapperByBos(context.getBosString())

      if (ChatWrapper != null) { this._promptWrapper = new ChatWrapper() } else { this._promptWrapper = new GeneralChatPromptWrapper() }
    } else { this._promptWrapper = promptWrapper }
  }

  /**
     * Gets whether the chat session has been initialized.
     * @returns {boolean} True if the chat session has been initialized, false otherwise.
     */
  get initialized () {
    return this._initialized
  }

  /**
     * Gets the context of the chat session.
     * @returns {Context} The context of the chat session.
     */
  get context () {
    return this._ctx
  }

  /**
     * Initializes the chat session.
     * @returns {Promise<void>} A promise that resolves when the initialization is complete.
     */
  async init () {
    await withLock(this, 'init', async () => {
      if (this._initialized) { return }

      if (this._printLLamaSystemInfo) { console.log(' system info', Model.systemInfo) }

      this._initialized = true
    })
  }

  /**
     * Generates a prompt and returns the generated text.
     * @param {string} prompt - The input prompt for the chat session.
     * @param {LLamaChatPromptOptions} options - The options for the chat prompt.
     * @returns {Promise<string>} A promise that resolves to the generated text.
     */
  async prompt (prompt, {
    onToken,
    signal,
    maxTokens,
    temperature,
    topK,
    topP,
    grammar = this.context._chatGrammar,
    trimWhitespaceSuffix = false,
    repeatPenalty
  } = {}) {
    const { text } = await this.promptWithMeta(prompt, {
      onToken, signal, maxTokens, temperature, topK, topP, grammar, trimWhitespaceSuffix, repeatPenalty
    })

    return text
  }

  /**
     * Generates a prompt with metadata and returns the generated text.
     * @param {string} prompt - The input prompt for the chat session.
     * @param {LLamaChatPromptOptions} options - The options for the chat prompt.
     * @returns {Promise<{text: string, stopReason: string, stopString: string, stopStringSuffix: string|null}>}
     * A promise that resolves to an object containing the generated text and metadata.
     */
  async promptWithMeta (prompt, {
    onToken,
    signal,
    maxTokens,
    temperature,
    topK,
    topP,
    grammar = this.context._chatGrammar,
    trimWhitespaceSuffix = false,
    repeatPenalty
  } = {}) {
    if (!this.initialized) { await this.init() }

    return await withLock(this, 'prompt', async () => {
      let promptText = ''

      if (this._promptIndex === 0 && this._conversationHistoryToLoad !== null) {
        const { text, stopString, stopStringSuffix } = generateContextTextFromConversationHistory(
          this._promptWrapper,
          this._conversationHistoryToLoad,
          {
            systemPrompt: this._systemPrompt,
            currentPromptIndex: this._promptIndex,
            lastStopString: this._lastStopString,
            lastStopStringSuffix: this._promptIndex === 0
              ? (
                  this._ctx.prependBos
                    ? this._ctx.getBosString()
                    : null
                )
              : this._lastStopStringSuffix
          }
        )

        promptText += text
        this._lastStopString = stopString
        this._lastStopStringSuffix = stopStringSuffix
        this._promptIndex += this._conversationHistoryToLoad.length

        this._conversationHistoryToLoad = null
      }

      promptText += this._promptWrapper.wrapPrompt(prompt, {
        systemPrompt: this._systemPrompt,
        promptIndex: this._promptIndex,
        lastStopString: this._lastStopString,
        lastStopStringSuffix: this._promptIndex === 0
          ? (
              this._ctx.prependBos
                ? this._ctx.getBosString()
                : null
            )
          : this._lastStopStringSuffix
      })
      this._promptIndex++
      this._lastStopString = null
      this._lastStopStringSuffix = null

      const encoded = this._ctx.encode({ text: promptText })
      const { text, stopReason, stopString, stopStringSuffix } = await this._evalTokens(encoded, {
          onToken,
          signal,
          maxTokens,
          temperature,
          topK,
          topP,
          grammar,
          trimWhitespaceSuffix,
          repeatPenalty: repeatPenalty === false ? { lastTokens: 0 } : repeatPenalty
        })

      this._lastStopString = stopString
      this._lastStopStringSuffix = stopStringSuffix

      return {
        text,
        stopReason,
        stopString,
        stopStringSuffix
      }
    })
  }

  /**
     * Evaluates tokens and returns the generated text and metadata.
     * @param {Uint32Array} tokens - The input tokens to evaluate.
     * @param {{onToken?: (tokens: Token[]) => void, signal?: AbortSignal, maxTokens?: number,
     * temperature?: number, topK?: number, topP?: number, grammar?: Grammar, trimWhitespaceSuffix?: boolean,
     * repeatPenalty?: ChatSessionRepeatPenalty}} options - The options for token evaluation.
     * @returns {Promise<{text: string, stopReason: string, stopString: string, stopStringSuffix: string|null}>}
     * A promise that resolves to an object containing the generated text and metadata.
     */
  async _evalTokens (tokens, {
    onToken,
    signal,
    maxTokens,
    temperature,
    topK,
    topP,
    grammar = this.context._chatGrammar,
    trimWhitespaceSuffix = false,
    repeatPenalty: {
      lastTokens: repeatPenaltyLastTokens = 64,
      punishTokensFilter,
      penalizeNewLine,
      penalty,
      frequencyPenalty,
      presencePenalty
    } = {}
  } = {}) {
    let stopStrings = this._promptWrapper.getStopStrings()

    if (grammar != null) { stopStrings = stopStrings.concat(grammar.stopStrings) }

    const stopStringIndexes = Array(stopStrings.length).fill(0)
    const skippedChunksQueue = []
    const res = []
    const grammarEvaluationState = grammar != null
      ? new GrammarEvaluationState({ grammar })
      : undefined
    const repeatPenaltyEnabled = repeatPenaltyLastTokens > 0
    let stopReason = 'eosToken'

    const getPenaltyTokens = () => {
      let punishTokens = res.slice(-repeatPenaltyLastTokens)

      if (punishTokensFilter != null) { punishTokens = punishTokensFilter(punishTokens) }

      if (!penalizeNewLine) {
        const nlToken = this.context.getNlToken()

        if (nlToken != null) { punishTokens = punishTokens.filter(token => token !== nlToken) }
      }

      return Uint32Array.from(punishTokens)
    }

    const evaluationIterator = this._ctx.eval(tokens, {
      tokens,
      temperature,
      topK,
      topP,
      grammarEvaluationState,
      repeatPenalty: !repeatPenaltyEnabled
        ? undefined
        : {
            punishTokens: getPenaltyTokens,
            penalty,
            frequencyPenalty,
            presencePenalty
          }
    })

    for await (const chunk of evaluationIterator) {
      if (signal?.aborted) { throw new Error() }

      const tokenStr = this._ctx.decode(Uint32Array.from([chunk]))
      const {
        shouldReturn, skipTokenEvent, stopString, stopStringSuffix
      } = this._checkStopString(tokenStr, stopStrings, stopStringIndexes)

      if (shouldReturn) {
        skippedChunksQueue.push(chunk)
        const skippedChunksText = skippedChunksQueue.length > 0
          ? this._ctx.decode(Uint32Array.from(skippedChunksQueue))
          : ''

        let [queuedTextBeforeStopString] = skippedChunksText.split(stopString)

        if (grammar?.trimWhitespaceSuffix || trimWhitespaceSuffix) { queuedTextBeforeStopString = queuedTextBeforeStopString.trimEnd() }

        if (queuedTextBeforeStopString.length > 0) {
          const beforeStopStringTokens = Array.from(this._ctx.encode(queuedTextBeforeStopString))

          res.push(...beforeStopStringTokens)
          onToken?.(beforeStopStringTokens)
          skippedChunksQueue.length = 0
        }

        stopReason = 'stopString'

        return {
          text: this._ctx.decode(Uint32Array.from(res)),
          stopReason,
          stopString,
          stopStringSuffix
        }
      }

      // If the token is unknown, it means it's not a complete character
      if (tokenStr === UNKNOWN_UNICODE_CHAR || skipTokenEvent || (
        (grammar?.trimWhitespaceSuffix || trimWhitespaceSuffix) && tokenStr.trim() === ''
      )) {
        skippedChunksQueue.push(chunk)
        continue
      }

      if (skippedChunksQueue.length > 0) {
        res.push(...skippedChunksQueue)
        onToken?.(skippedChunksQueue)
        skippedChunksQueue.length = 0
      }

      res.push(chunk)
      onToken?.([chunk])

      if (maxTokens != null && maxTokens > 0 && res.length >= maxTokens) {
        stopReason = 'maxTokens'
        break
      }
    }

    let resText = this._ctx.decode(Uint32Array.from(res))

    if (grammar?.trimWhitespaceSuffix || trimWhitespaceSuffix) { resText = resText.trimEnd() }

    return {
      text: resText,
      stopReason,
      stopString: null,
      stopStringSuffix: null
    }
  }

  /**
     * Checks if the token string matches any stop string and returns metadata.
     * @param {string} tokenStr - The token string to check.
     * @param {string[]} stopStrings - An array of stop strings to match against.
     * @param {number[]} stopStringIndexes - An array of stop string indexes for matching.
     * @returns {{shouldReturn: boolean, skipTokenEvent: boolean, stopString: string, stopStringSuffix: string|null}}
     * An object containing metadata about the token string.
     */
  _checkStopString (tokenStr, stopStrings, stopStringIndexes) {
    let skipTokenEvent = false

    for (let stopStringIndex = 0; stopStringIndex < stopStrings.length; stopStringIndex++) {
      const stopString = stopStrings[stopStringIndex]

      let localShouldSkipTokenEvent = false
      let i = 0
      for (; i < tokenStr.length && stopStringIndexes[stopStringIndex] !== stopString.length; i++) {
        if (tokenStr[i] === stopString[stopStringIndexes[stopStringIndex]]) {
          stopStringIndexes[stopStringIndex]++
          localShouldSkipTokenEvent = true
        } else {
          stopStringIndexes[stopStringIndex] = 0
          localShouldSkipTokenEvent = false
        }
      }

      if (stopStringIndexes[stopStringIndex] === stopString.length) {
        return {
          shouldReturn: true,
          stopString,
          stopStringSuffix: tokenStr.length === i
            ? null
            : tokenStr.slice(i)
        }
      }

      skipTokenEvent ||= localShouldSkipTokenEvent
    }

    return { skipTokenEvent }
  }
}

/**
 * Generate context text to load into a model context from a conversation history.
 * @param {ChatPromptWrapper} chatPromptWrapper - The chat prompt wrapper.
 * @param {ConversationInteraction[]} conversationHistory - The conversation history.
 * @param {object} [options] - Additional options.
 * @param {string} [options.systemPrompt] - The system prompt.
 * @param {number} [options.currentPromptIndex] - The current prompt index.
 * @param {string | null} [options.lastStopString] - The last stop string.
 * @param {string | null} [options.lastStopStringSuffix] - The last stop string suffix.
 * @returns {{text: string, stopString: (string | null), stopStringSuffix: (string | null)}}
 * An object containing the generated text, stop string, and stop string suffix.
 */
function generateContextTextFromConversationHistory (
  chatPromptWrapper,
  conversationHistory,
  {
    systemPrompt = defaultChatSystemPrompt,
    currentPromptIndex = 0,
    lastStopString = null,
    lastStopStringSuffix = null
  } = {}
) {
  let res = ''

  for (let i = 0; i < conversationHistory.length; i++) {
    const interaction = conversationHistory[i]
    const wrappedPrompt = chatPromptWrapper.wrapPrompt(interaction.prompt, {
      systemPrompt,
      promptIndex: currentPromptIndex,
      lastStopString,
      lastStopStringSuffix
    })
    const stopStrings = chatPromptWrapper.getStopStrings()
    const defaultStopString = chatPromptWrapper.getDefaultStopString()
    const stopStringsToCheckInResponse = new Set([...stopStrings, defaultStopString])

    currentPromptIndex++
    lastStopString = null
    lastStopStringSuffix = null

    res += wrappedPrompt

    for (const stopString of stopStringsToCheckInResponse) {
      if (interaction.response.includes(stopString)) {
        console.error(
                    `Stop string "${stopString}" was found in model response of conversation interaction index ${i}`,
                    { interaction, stopString }
        )
        throw new Error('A stop string cannot be in a conversation history interaction model response')
      }
    }

    res += interaction.response
    res += defaultStopString
    lastStopString = defaultStopString
    lastStopStringSuffix = ''
  }

  return {
    text: res,
    stopString: lastStopString,
    stopStringSuffix: lastStopStringSuffix
  }
}

export {
  ChatPromptWrapper,
  ChatMLChatPromptWrapper,
  GeneralChatPromptWrapper
}
