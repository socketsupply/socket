/* global EventTarget */
import { HotKeyEvent } from '../internal/events.js'
import ipc from '../ipc.js'
import os from '../os.js'

if (!/android|ios/.test(os.platform())) {
  globalThis.addEventListener('hotkey', onHotKey)
}

/**
 * Global `HotKeyEvent` event listener for `Binding` instance event dispatch.
 * @ignore
 * @param {import('../internal/events.js').HotKeyEvent} event
 */
function onHotKey (event) {
  console.log({ event })
  const { id } = event.data ?? {}
  const binding = bindings.get(id)?.deref?.()
  if (binding) {
    binding.dispatchEvent(new HotKeyEvent('hotkey', event.data))
  }
}

/**
 * Normalizes an expression string.
 * @param {string} expression
 * @return {string}
 */
export function normalizeExpression (expression) {
  return encodeURIComponent(expression.split('+').map((token) => token.trim()).join('+'))
}

/**
 * A map of weakly held `Binding` instances.
 * @type {Map<number, WeakRef<Binding>>}
 */
export const bindings = new Map()

/**
 * An `EventTarget` container for a hotkey binding.
 */
export class Binding extends EventTarget {
  /**
   * The global unique ID for this binding.
   * @ignore
   * @type {number?}
   */
  #id = null

  /**
   * The computed hash for this binding expression.
   * @ignore
   * @type {number?}
   */
  #hash = null

  /**
   * The normalized expression as a sequence of tokens.
   * @ignore
   * @type {string[]}
   */
  #sequence = []

  /**
   * The original expression of the binding.
   * @ignore
   * @type {string?}
   */
  #expression = null

  /**
   * `Binding` class constructor.
   * @ignore
   * @param {object} data
   */
  constructor (data) {
    super()
    if (data?.id && typeof data.id === 'number') {
      this.#id = data.id
    }

    if (data?.hash && typeof data.hash === 'number') {
      this.#hash = data.hash
    }

    if (Array.isArray(data?.sequence)) {
      this.#sequence = Object.freeze(Array.from(data.sequence))
    }

    if (data?.expression && typeof data.expression === 'string') {
      this.#expression = data.expression
    }

    if (this.isValid) {
      bindings.set(this.id, new WeakRef(this))
    }
  }

  /**
   * `true` if the binding is valid, otherwise `false.
   * @type {boolean}
   */
  get isValid () {
    return this.id && this.hash && this.expression && this.sequence.length
  }

  /**
   * The global unique ID for this binding.
   * @type {number?}
   */
  get id () {
    return this.#id
  }

  /**
   * The computed hash for this binding expression.
   * @type {number?}
   */
  get hash () {
    return this.#hash
  }

  /**
   * The normalized expression as a sequence of tokens.
   * @type {string[]}
   */
  get sequence () {
    return this.#sequence
  }

  /**
   * The original expression of the binding.
   * @type {string?}
   */
  get expression () {
    return this.#expression
  }

  /**
   * Unbinds this hotkey expression.
   * @param {object=} poptions]
   * @return {Promise}
   */
  async unbind (options = null) {
    return await unbind(this.id, options)
  }
}

/**
 * Bind a global hotkey expression.
 * @param {string} expression
 * @param {object=} [options]
 * @return {Promise<Binding>}
 */
export async function bind (expression, options = null) {
  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  expression = normalizeExpression(expression)
  const result = await ipc.request('window.hotkey.bind', { expression }, options)

  if (result.err) {
    throw result.err
  }

  return bindings.get(result.data.id)?.deref() ?? new Binding(result.data)
}

/**
 * Bind a global hotkey expression.
 * @param {string} expression
 * @param {object=} [options]
 * @return {Promise<Binding>}
 */
export async function unbind (id, options = null) {
  let result = null

  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  if (typeof id === 'string') {
    const expression = normalizeExpression(/* @type {string} */ (id))
    result = await ipc.request('window.hotkey.unbind', { expression }, options)
  } else {
    result = await ipc.request('window.hotkey.unbind', { id }, options)
  }

  if (result.err) {
    throw result.err
  }

  if (result.data?.id) {
    bindings.delete(result.data.id)
  }
}

/**
 * Get all known globally register hotkey bindings.
 * @param {object=} [options]
 * @return {Promise<Binding[]>}
 */
export async function getBindings (options = null) {
  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  const result = await ipc.request('window.hotkey.bindings', {}, options)

  if (result.err) {
    throw result.err
  }

  if (Array.isArray(result.data)) {
    return result.data.map((data) =>
      bindings.get(data.id)?.deref?.() ?? new Binding(data)
    )
  }

  return []
}

/**
 * Get all known possible keyboard modifier and key mappings for
 * expression bindings.
 * @param {object=} [options]
 * @return {Promise<{ keys: object, modifiers: object }>}
 */
export async function getMappings (options = null) {
  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  const result = await ipc.request('window.hotkey.mappings', {}, options)

  if (result.err) {
    throw result.err
  }

  return result.data ?? {}
}

export default {
  bind,
  unbind
}
