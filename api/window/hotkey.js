/* global Event, EventTarget, ErrorEvent */
import { HotKeyEvent } from '../internal/events.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'
import gc from '../gc.js'
import os from '../os.js'

/**
 * Normalizes an expression string.
 * @param {string} expression
 * @return {string}
 */
export function normalizeExpression (expression) {
  return encodeURIComponent(expression
    .split('+')
    .map((token) => token.trim())
    .join('+')
  )
}

/**
 * A high level bindings container map that dispatches events.
 */
export class Bindings extends EventTarget {
  /**
   * A map of weakly held `Binding` instances.
   * @ignore
   * @type {Map<number, WeakRef<Binding>>}
   */
  #map = new Map()

  /**
   * A `BroadcastChannel` for hot key bindings
   * @ignore
   * @type {BroadcastChannel}
   */
  #channel = new BroadcastChannel('socket.runtime.window.hotkey.bindings')

  /**
   * The source `EventTarget` to listen for 'hotkey' events on
   * @ignore
   * @type {EventTarget}
   */
  #sourceEventTarget = null

  /**
   * Level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  #onerror = null

  /**
   * Level 1 'hotkey'` event listener.
   * @ignore
   * @type {function(HotKeyEvent)?}
   */
  #onhotkey = null

  #clock = globalThis.performance.now()

  /**
   * `Bindings` class constructor.
   * @ignore
   * @param {EventTarget} [sourceEventTarget]
   */
  constructor (sourceEventTarget = globalThis) {
    super()

    this.onHotKey = this.onHotKey.bind(this)
    this.#sourceEventTarget = sourceEventTarget
    this.#channel.addEventListener('message', (event) => {
      this.onHotKey(event)
    })

    if (!/android|ios/.test(os.platform())) {
      sourceEventTarget.addEventListener('hotkey', this.onHotKey)
      sourceEventTarget.addEventListener('hotkey', (event) => {
        const { id } = event.data ?? {}
        if (!this.has(id)) {
          this.#channel.postMessage(event.data)
        }
      })
    }

    gc.ref(this)
    this.init()
  }

  /**
   * The number of `Binding` instances in the mapping.
   * @type {number}
   */
  get size () {
    return this.#map.size
  }

  /**
   * Level 1 'error'` event listener.
   * @type {function(ErrorEvent)?}
   */
  get onerror () {
    return this.#onerror ?? null
  }

  /**
   * Setter for the level 1 'error'` event listener.
   * @ignore
   * @type {function(ErrorEvent)?}
   */
  set onerror (onerror) {
    if (this.#onerror) {
      this.removeEventListener('error', this.#onerror)
    }

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  /**
   * Level 1 'hotkey'` event listener.
   * @type {function(hotkeyEvent)?}
   */
  get onhotkey () {
    return this.#onhotkey ?? null
  }

  /**
   * Setter for the level 1 'hotkey'` event listener.
   * @ignore
   * @type {function(HotKeyEvent)?}
   */
  set onhotkey (onhotkey) {
    if (this.#onhotkey) {
      this.removeEventListener('hotkey', this.#onhotkey)
    }

    if (typeof onhotkey === 'function') {
      this.#onhotkey = onhotkey
      this.addEventListener('hotkey', onhotkey)
    }
  }

  /**
   * Initializes bindings from global context.
   * @ignore
   * @return {Promise}
   */
  async init () {
    try {
      for (const binding of await getBindings()) {
        await binding.bind()
      }
    } catch (error) {
      this.dispatchEvent(new ErrorEvent('error', { error }))
    }
  }

  /**
   * Global `HotKeyEvent` event listener for `Binding` instance event dispatch.
   * @ignore
   * @param {import('../internal/events.js').HotKeyEvent} event
   */
  onHotKey (event) {
    const { id } = event.data ?? {}
    const binding = this.get(id)

    if (event.timeStamp < this.#clock) {
      return false
    }

    this.#clock = event.timeStamp

    if (binding) {
      binding.dispatchEvent(new HotKeyEvent('hotkey', event.data))
    }

    const e = new HotKeyEvent('hotkey', event.data)
    e.binding = this.get(id) ?? new Binding(event.data)
    this.dispatchEvent(e)
  }

  /**
   * Get a binding by `id`
   * @param {number} id
   * @return {Binding}
   */
  get (id) {
    return this.#map.get(id)?.deref?.()
  }

  /**
   * Set a `binding` a by `id`.
   * @param {number} id
   * @param {Binding} binding
   */
  set (id, binding) {
    this.#map.set(id, new WeakRef(binding))
  }

  /**
   * Delete a binding by `id`
   * @param {number} id
   * @return {boolean}
   */
  delete (id) {
    return this.#map.delete(id)
  }

  /**
   * Returns `true` if a binding exists in the mapping, otherwise `false`.
   * @return {boolean}
   */
  has (id) {
    return this.#map.has(id)
  }

  /**
   * Known `Binding` values in the mapping.
   * @return {{ next: function(): { value: Binding|undefined, done: boolean } }}
   */
  values () {
    const values = Array
      .from(this.#map.values())
      .map((ref) => ref.deref() ?? null)
      .filter(Boolean)
    return values[Symbol.iterator]()
  }

  /**
   * Known `Binding` keys in the mapping.
   * @return {{ next: function(): { value: number|undefined, done: boolean } }}
   */
  keys () {
    return this.#map.keys()
  }

  /**
   * Known `Binding` ids in the mapping.
   * @return {{ next: function(): { value: number|undefined, done: boolean } }}
   */
  ids () {
    return this.keys()
  }

  /**
   * Known `Binding` ids and values in the mapping.
   * @return {{ next: function(): { value: [number, Binding]|undefined, done: boolean } }}
   */
  entries () {
    const entries = Array
      .from(this.#map.entries())
      .map(([id, ref]) => [id, ref.deref() ?? null])
      .filter((entry) => entry[1])
    return entries[Symbol.iterator]()
  }

  /**
   * Bind a global hotkey expression.
   * @param {string} expression
   * @return {Promise<Binding>}
   */
  async bind (expression) {
    return await bind(expression)
  }

  /**
   * Bind a global hotkey expression.
   * @param {string} expression
   * @return {Promise<Binding>}
   */
  async unbind (expression) {
    return await unbind(expression)
  }

  /**
   * Returns an array of all active bindings for the application.
   * @return {Promise<Binding[]>}
   */
  async active () {
    return await getBindings()
  }

  /**
   * Resets all active bindings in the application.
   * @param {boolean=} [currentContextOnly]
   * @return {Promise}
   */
  async reset (currentContextOnly = false) {
    if (currentContextOnly) {
      for (const binding of this.values()) {
        await binding.unbind()
      }
    } else {
      const active = await this.active()
      for (const binding of active) {
        await binding.unbind()
      }
    }
  }

  /**
   * Implements the `Iterator` protocol for each currently registered
   * active binding in this window context. The `AsyncIterator` protocol
   * will probe for all gloally active bindings.
   * @return {Iterator<Binding>}
   */
  [Symbol.iterator] () {
    return this.values()
  }

  /**
   * Implements the `AsyncIterator` protocol for each globally active
   * binding registered to the application. This differs from the `Iterator`
   * protocol as this will probe for _all_ active bindings in the entire
   * application context.
   * @return {AsyncGenerator<Binding>}
   */
  async * [Symbol.asyncIterator] () {
    for (const binding of await this.active()) {
      yield binding
    }
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.#map, this.#sourceEventTarget, this.onHotKey],
      handle (map, sourceEventTarget, onHotKey) {
        map.clear()
        if (sourceEventTarget) {
          sourceEventTarget.removeEventListener('hotkey', onHotKey)
        }
      }
    }
  }
}

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
   * Level 1 'hotkey'` event listener.
   * @ignore
   * @type {function(HotKeyEvent)?}
   */
  #onhotkey = null

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
      bindings.set(this.id, this)
    }
  }

  /**
   * `true` if the binding is valid, otherwise `false`.
   * @type {boolean}
   */
  get isValid () {
    return this.id && this.hash && this.expression && this.sequence.length
  }

  /**
   * `true` if the binding is considered active, otherwise `false`.
   * @type {boolean}
   */
  get isActive () {
    return this.isValid && bindings.has(this.id)
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
   * Level 1 'hotkey'` event listener.
   * @type {function(hotkeyEvent)?}
   */
  get onhotkey () {
    return this.#onhotkey ?? null
  }

  /**
   * Setter for the level 1 'hotkey'` event listener.
   * @ignore
   * @type {function(HotKeyEvent)?}
   */
  set onhotkey (onhotkey) {
    if (this.#onhotkey) {
      this.removeEventListener('hotkey', this.#onhotkey)
    }

    if (typeof onhotkey === 'function') {
      this.#onhotkey = onhotkey
      this.addEventListener('hotkey', onhotkey)
    }
  }

  /**
   * Binds this hotkey expression.
   * @return {Promise<Binding>}
   */
  async bind () {
    return await bind(this.id)
  }

  /**
   * Unbinds this hotkey expression.
   * @return {Promise}
   */
  async unbind () {
    await unbind(this.id)
  }

  /**
   * Implements the `AsyncIterator` protocol for async 'hotkey' events
   * on this binding instance.
   * @return {AsyncGenerator}
   */
  async * [Symbol.asyncIterator] () {
    while (this.isActive) {
      yield await new Promise((resolve) => {
        this.addEventListener(
          'hotkey',
          (event) => queueMicrotask(() => resolve(event.data)),
          { once: true }
        )
      })
    }
  }
}

/**
 * Bind a global hotkey expression.
 * @param {string} expression
 * @param {{ passive?: boolean }} [options]
 * @return {Promise<Binding>}
 */
export async function bind (expression, options = null) {
  const params = {}

  if (typeof options?.passive === 'boolean') {
    params.passive = options.passive
  }

  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  await hooks.wait('ready')

  if (typeof expression === 'number') {
    params.id = /** @type {number} */ (expression)
  } else if (typeof expression === 'string') {
    params.expression = normalizeExpression(expression)
  } else {
    throw new TypeError('Expecting expression argument to be a string')
  }

  const result = await ipc.request('window.hotkey.bind', params, options)

  if (result.err) {
    throw result.err
  }

  if (bindings.has(result.data.id)) {
    return bindings.get(result.data.id)
  }

  const binding = new Binding(result.data)
  binding.dispatchEvent(new Event('bind'))
  return binding
}

/**
 * Bind a global hotkey expression.
 * @param {string} expression
 * @param {object=} [options]
 * @return {Promise<Binding>}
 */
export async function unbind (id, options = null) {
  const params = {}

  await hooks.wait('ready')

  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  if (typeof id === 'number') {
    params.id = id
  } else if (typeof id === 'string') {
    params.expression = normalizeExpression(/** @type {string} */ (id))
  } else {
    throw new TypeError('Expecting expression argument to be a string')
  }

  const result = await ipc.request('window.hotkey.unbind', params, options)

  if (result.err) {
    throw result.err
  }

  if (!result.data?.id) {
    return
  }

  if (bindings.has(result.data.id)) {
    const binding = bindings.get(result.data.id)
    binding.dispatchEvent(new Event('unbind'))
    bindings.delete(binding.id)
  }
}

/**
 * Get all known globally register hotkey bindings.
 * @param {object=} [options]
 * @return {Promise<Binding[]>}
 */
export async function getBindings (options = null) {
  await hooks.wait('ready')

  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  const result = await ipc.request('window.hotkey.bindings', {}, options)

  if (result.err) {
    throw result.err
  }

  if (Array.isArray(result.data)) {
    return result.data.map((data) =>
      bindings.get(data.id) ?? new Binding(data)
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
  await hooks.wait('ready')

  if (/android|ios/.test(os.platform())) {
    throw new TypeError(`The HotKey API is not supported on '${os.platform()}'`)
  }

  const result = await ipc.request('window.hotkey.mappings', {}, options)

  if (result.err) {
    throw result.err
  }

  return result.data ?? {}
}

/**
 * Adds an event listener to the global active bindings. This function is just
 * proxy to `bindings.addEventListener`.
 * @param {string} type
 * @param {function(Event)} listener
 * @param {(boolean|object)=} [optionsOrUseCapture]
 */
export function addEventListener (type, listener, optionsOrUseCapture) {
  return bindings.addEventListener(type, listener, optionsOrUseCapture)
}

/**
 * Removes  an event listener to the global active bindings. This function is
 * just a proxy to `bindings.removeEventListener`
 * @param {string} type
 * @param {function(Event)} listener
 * @param {(boolean|object)=} [optionsOrUseCapture]
 */
export function removeEventListener (type, listener, optionsOrUseCapture) {
  return bindings.removeEventListener(type, listener, optionsOrUseCapture)
}

/**
 * A container for all the bindings currently bound
 * by this window context.
 * @type {Bindings}
 */
export const bindings = new Bindings()

export default bindings
