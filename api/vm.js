/**
 * @module VM
 *
 * This module enables compiling and running JavaScript source code in an
 * isolated execution context optionally with a user supplied context object.
 *
 * Example usage:
 * ```js
 * import fs from 'socket:fs/promises'
 * import vm from 'socket:vm'
 *
 * const data = await fs.readFile('data.json')
 * const context = { data, value: null }
 * const source = `
 *   const text = new TextDecoder().decode(data)
 *   const json = JSON.parse(text)
 *
 *   // get `.value` from parsed JSON and set it to global `value`
 *   // that exists on the user context
 *   value = json.value
 * `
 * const result = await vm.runInContext(source, context)
 * console.log(context.value) // set from `json.value` in VM context
 * ```
 */

/* global ErrorEvent, EventTarget, MessagePort */
import { maybeMakeError } from './ipc.js'
import { SharedWorker } from './internal/shared-worker.js'
import { detectESMSource } from './commonjs/package.js'
import application from './application.js'
import globals from './internal/globals.js'
import process from './process.js'
import console from './console.js'
import crypto from './crypto.js'
import os from './os.js'
import gc from './gc.js'

const AsyncFunction = (async () => {}).constructor
const Uint8ArrayPrototype = Uint8Array.prototype
const TypedArrayPrototype = Object.getPrototypeOf(Uint8ArrayPrototype)
const TypedArray = TypedArrayPrototype.constructor

const kContextTag = Symbol('socket.vm.Context')

const VM_WINDOW_INDEX = 47
const VM_WINDOW_TITLE = 'socket:vm'
const VM_WINDOW_PATH = `${globalThis.origin}/socket/vm/index.html`

let contextWorker = null
let contextWindow = null

// A weak mapping of context objects to `Script` instances where "context"
// objects own the `Script` until the "context" is no longer stronglyheld
// in which the `Script` eventually becomes garbage collected triggering
// the `gc.finalizer` callback to be invoked cleaning up any allocated
// resources for the script context such as iframes and workers or any
// resources created in the script "world" in the VM realm
const scripts = new WeakMap()

// A weak mapping of created contexts
const contexts = new WeakMap()
// a weak mapping of created global objects
const globalObjects = new WeakMap()

// a shared context when one is not given
const sharedContext = createContext({})

// blob URL caches key by content hash
const blobURLCache = new Map()

// A weak mapping of values to reference objects
const references = Object.assign(new WeakMap(), {
  // A mapping of reference IDs to weakly held `Reference` instances
  index: new Map()
})

function isTypedArray (object) {
  return object instanceof TypedArray
}

function isArrayBuffer (object) {
  return object instanceof ArrayBuffer
}

function convertSourceToString (source) {
  if (source && typeof source !== 'string') {
    if (typeof source.valueOf === 'function') {
      source = source.valueOf()
    }

    if (typeof source.toString === 'function') {
      source = source.toString()
    }
  }

  if (typeof source !== 'string') {
    throw new TypeError(
      'Expecting Script source to be a string ' +
      `or a value that can be converted to one. Received: ${source}`
    )
  }

  return source
}

/**
 * @ignore
 * @param {object[]} transfer
 * @param {object} object
 * @param {object=} [options]
 * @return {object[]}
 */
export function findMessageTransfers (transfers, object, options = null) {
  if (isTypedArray(object) || ArrayBuffer.isView(object)) {
    add(object.buffer)
  } else if (isArrayBuffer(object)) {
    add(object)
  } else if (Array.isArray(object)) {
    for (const value of object) {
      findMessageTransfers(transfers, value, options)
    }
  } else if (object && typeof object === 'object') {
    if (
      object instanceof MessagePort || (
        typeof object.postMessage === 'function' &&
        Object.getPrototypeOf(object).constructor.name === 'MessagePort'
      )
    ) {
      add(object)
    } else {
      for (const key in object) {
        if (
          key.startsWith('__vmScriptReferenceArgs_') &&
          options?.ignoreScriptReferenceArgs === true
        ) {
          continue
        }

        findMessageTransfers(transfers, object[key], options)
      }
    }
  }

  return transfers

  function add (value) {
    if (!transfers.includes(value)) {
      transfers.push(value)
    }
  }
}

/**
 * @ignore
 * @param {object} context
 */
export function applyInputContextReferences (context) {
  if (!context || typeof context !== 'object') {
    return
  }

  visitObject(context)

  function visitObject (object) {
    for (const key in object) {
      const value = object[key]
      if (value && typeof value === 'object') {
        if (value.__vmScriptReference__ === true && value.id) {
          const reference = getReference(value.id)
          if (reference) {
            object[key] = reference.value
            Object.defineProperty(context, value.id, {
              configurable: false,
              enumerable: false,
              value: reference.value
            })
          }
        } else {
          visitObject(value)
        }
      }
    }
  }
}

/**
 * @ignore
 * @param {object} context
 */
export function applyOutputContextReferences (context) {
  if (!context || typeof context !== 'object') {
    return
  }

  visitObject(context)

  function visitObject (object) {
    if (object.__vmScriptReference__ && 'value' in object) {
      object = object.value
    }

    if (!object || typeof object !== 'object') {
      return
    }

    const keys = new Set(Object.keys(object))
    if (
      object &&
      typeof object === 'object' &&
      !(object instanceof Reference) &&
      Object.getPrototypeOf(object) !== Object.prototype &&
      Object.getPrototypeOf(object) !== Array.prototype
    ) {
      const prototype = Object.getPrototypeOf(object)
      if (prototype) {
        const descriptors = Object.getOwnPropertyDescriptors(prototype)
        if (descriptors) {
          for (const key of Object.keys(descriptors)) {
            if (key !== 'constructor') {
              keys.add(key)
            }
          }
        }
      }
    }

    for (const key of keys) {
      if (key.startsWith('__vmScriptReferenceArgs_')) {
        Reflect.deleteProperty(object, key)
        continue
      }

      let value = object[key]
      if (value && typeof value === 'object') {
        if (Symbol.toStringTag in value) {
          const tag = typeof value[Symbol.toStringTag] === 'function'
            ? value[Symbol.toStringTag]()
            : value[Symbol.toStringTag]

          if (tag === 'Module') {
            value = object[key] = { ...value }
          }
        }

        if (!(value.__vmScriptReference__ === true && value.id)) {
          visitObject(value)
        }
      }

      if (typeof value === 'function') {
        const reference = getReference(value) ?? createReference(value, context)
        object[key] = reference.toJSON()
      }
    }
  }
}

/**
 * @ignore
 * @param {object} context
 */
export function filterNonTransferableValues (context) {
  if (!context || typeof context !== 'object') {
    return
  }

  visitObject(context)

  function visitObject (object) {
    for (const key in object) {
      const value = object[key]
      if (value && typeof value === 'object') {
        visitObject(value)
      } else if (typeof value === 'function') {
        const reference = getReference(value)
        if (reference) {
          object[key] = reference.toJSON()
        } else {
          Reflect.deleteProperty(object, key)
        }
      }
    }
  }
}

/**
 * @ignore
 * @param {object=} [currentContext]
 * @param {object=} [updatedContext]
 * @param {object=} [contextReference]
 * @return {{ deletions: string[], merges: string[] }}
 */
export function applyContextDifferences (
  currentContext = null,
  updatedContext = null,
  contextReference = null,
  preserveScriptArgs = false
) {
  if (!currentContext || typeof currentContext !== 'object') {
    return
  }

  const deletions = []
  const merges = []
  const script = scripts.get(contextReference ?? currentContext)

  for (const key in updatedContext) {
    const updatedValue = Reflect.get(updatedContext, key)

    if (updatedValue?.__vmScriptReference__ === true) {
      const reference = updatedValue

      if (reference.type === 'function') {
        const ref = getReference(reference.id)
        if (ref) {
          Reflect.set(currentContext, key, ref.value)
        } else if (script) {
          const container = {
            [key]: function (...args) {
              const isConstructorCall = this instanceof container[key]
              const scriptReferenceArgsKey = `__vmScriptReferenceArgs_${reference.id}__`

              Reflect.set(contextReference, scriptReferenceArgsKey, args)
              Reflect.set(contextReference, reference.id, reference)

              const promise = new Promise((resolve, reject) => {
                const promise = script.runInContext(contextReference, {
                  mode: 'classic',
                  source: `${isConstructorCall ? 'new ' : ''}globalObject['${reference.id}'](...globalObject['${scriptReferenceArgsKey}'])`
                })

                promise.then(resolve).catch(reject)
              })

              promise.finally(() => {
                Reflect.deleteProperty(contextReference, reference.id)
                Reflect.deleteProperty(contextReference, scriptReferenceArgsKey)
              })

              if (!isConstructorCall) {
                return promise.then((result) => {
                  if (result?.__vmScriptReference__ === true) {
                    return result.value
                  }

                  return result
                })
              }

              return new Proxy(function () {}, {
                get (target, property, receiver) {
                  return new Proxy(function () {}, {
                    apply (target, __, argumentList) {
                      return apply(promise)
                      function apply (result) {
                        if (!result?.then) {
                          return result
                        }

                        return result
                          .then((result) => {
                            if (result?.value) {
                              applyContextDifferences(result, result, contextReference)
                              if (typeof result.value === 'object' && property in result.value) {
                                if (typeof result.value[property] === 'function') {
                                  return result.value[property](...argumentList)
                                }

                                return result.value[property]
                              }

                              return result.value
                            } else {
                              return result
                            }
                          })
                          .then(apply)
                      }
                    }
                  })
                },
                apply (target, thisArg, argumentList) {
                  return promise
                    .then((result) => typeof result === 'function'
                      ? result(...args)
                      : result
                    )
                    .then((result) => typeof result === 'function'
                      ? isConstructorCall
                        ? result.call(thisArg, ...argumentList)
                        : result.bind(thisArg, ...argumentList)
                      : result
                    )
                    .then((result) => {
                      applyContextDifferences(result, result, contextReference)
                      return result
                    })
                }
              })
            }
          }

          // wrap into container for named function, called in tail with an
          // intentional omission of `await` for an async call stack collapse
          // this preserves naming in `console.log`:
          //   [AsyncFunction: functionName]
          // while also removing an unneeded tail call in a stack trace
          const containerForNamedFunction = {
            [key]: function (...args) {
              if (this instanceof containerForNamedFunction[key]) {
                return new container[key](...args)
              }

              return container[key].call(this, ...args)
            }
          }

          // the reference ID was created on the other side, just use it here
          // instead of creating a new one which will preserve the binding
          // between this caller context and the realm world where the script
          // execution is actually occuring
          putReference(new Reference(
            reference.id,
            containerForNamedFunction[key],
            contextReference,
            { external: true }
          ))

          // emplace an actual function on `currentContext` at the property
          // `key` which will do the actual proxy call to the VM script
          Reflect.set(currentContext, key, containerForNamedFunction[key])
        }
      }
    } else if (Reflect.has(currentContext, key)) {
      const currentValue = Reflect.get(currentContext, key)
      if (
        currentValue && typeof currentValue === 'object' &&
        updatedValue && typeof updatedValue === 'object'
      ) {
        merges.push([key, currentValue, updatedValue])
      } else {
        Reflect.set(currentContext, key, updatedValue)
      }
    } else {
      Reflect.set(currentContext, key, updatedValue)
    }
  }

  for (const key in currentContext) {
    if (!preserveScriptArgs && key.startsWith('__vmScriptReferenceArgs_')) {
      Reflect.deleteProperty(currentContext, key)
    }

    if (!Reflect.has(updatedContext, key)) {
      deletions.push(key)
      Reflect.deleteProperty(currentContext, key)
    }
  }

  for (const merge of merges) {
    const [key, currentValue, updatedValue] = merge
    if (isTypedArray(updatedValue) || isArrayBuffer(updatedValue)) {
      Reflect.set(currentContext, key, updatedValue)
    } else if (!Array.isArray(currentValue) && Array.isArray(updatedValue)) {
      Reflect.set(currentContext, key, updatedValue)
    } else if (Array.isArray(currentValue) && Array.isArray(updatedValue)) {
      currentValue.length = updatedValue.length
      for (let i = 0; i < updatedValue.length; ++i) {
        Reflect.set(currentValue, i, Reflect.get(updatedValue, i))
      }
    } else {
      applyContextDifferences(currentValue, updatedValue, contextReference)
    }
  }

  return { deletions, merges: merges.map((merge) => merge[0]) }
}

/**
 * Wrap a JavaScript function source.
 * @ignore
 * @param {string} source
 * @param {object=} [options]
 */
export function wrapFunctionSource (source, options = null) {
  source = source.trim()
  if (
    source.startsWith('{') ||
    source.startsWith('async') ||
    source.startsWith('function') ||
    source.startsWith('class')
  ) {
    source = `(${source})`
  } else if (source.includes('return') || source.includes(';') || source.includes('throw')) {
    source = `{\n${source}\n}`
  } else if (source.includes('\n')) {
    const parts = source.trim().split('\n')
    const last = parts.pop()
    const tmp = last.trim()
    if (
      !/^(if|return|while|do|switch|let|var|const|for)/.test(tmp) &&
      !/^[{|;|/]/.test(tmp) &&
      !/}$/.test(tmp) &&
      !/\w\s*=\*\w/.test(tmp)
    ) {
      source = parts.concat(`return ${last}`).join('\n')
    }

    source = `{\n${source}\n}`
  }

  return `
    with (this) { return (${options?.async ? 'async' : ''} (arguments) => ${source})(typeof arguments !== 'undefined' ? arguments : []); }
    //# sourceURL=${options?.filename || 'wrapped-function-source.js'}
  `.trim()
}

/**
 * A container for a context worker message channel that looks like a "worker".
 * @ignore
 */
export class ContextWorkerInterface extends EventTarget {
  #channel = null

  constructor () {
    super()

    this.#channel = new MessageChannel()
  }

  get channel () {
    return this.#channel
  }

  get port () {
    return this.#channel.port1
  }

  destroy () {
    try {
      this.#channel.port1.close()
      this.#channel.port2.close()
    } catch {}

    this.#channel = null
  }
}

/**
 * A container proxy for a context worker message channel that
 * looks like a "worker".
 * @ignore
 */
export class ContextWorkerInterfaceProxy extends EventTarget {
  #globals = null
  constructor (globals) {
    super()
    this.#globals = globals
    gc.ref(this)
  }

  get port () {
    return this.#globals.get('vm.contextWorker')?.channel?.port2
  }

  [gc.finalizer] () {
    return {
      args: [this.port],
      async handle (port) {
        if (port) {
          try {
            port.close()
          } catch {}
        }
      }
    }
  }
}

/**
 * Global reserved values that a script context may not modify.
 * @type {string[]}
 */
export const RESERVED_GLOBAL_INTRINSICS = [
  '__args',
  '__globals',
  'top',
  'self',
  'this',
  'window',
  'webkit',
  'chrome',
  'external',
  'postMessage',
  'Infinity',
  'NaN',
  'undefined',
  'eval',
  'isFinite',
  'isNaN',
  'parseFloat',
  'parseInt',
  'decodeURI',
  'decodeURIComponent',
  'encodeURI',
  'encodeURIComponent',
  'AggregateError',
  'Array',
  'ArrayBuffer',
  'Atomics',
  'BigInt',
  'BigInt64Array',
  'BigUint64Array',
  'Boolean',
  'DataView',
  'Date',
  'Error',
  'EvalError',
  'FinalizationRegistry',
  'Float32Array',
  'Float64Array',
  'Function',
  'Int8Array',
  'Int16Array',
  'Int32Array',
  'Map',
  'Number',
  'Object',
  'Promise',
  'Proxy',
  'RangeError',
  'ReferenceError',
  'RegExp',
  'Set',
  'SharedArrayBuffer',
  'String',
  'Symbol',
  'SyntaxError',
  'TypeError',
  'Uint8Array',
  'Uint8ClampedArray',
  'Uint16Array',
  'Uint32Array',
  'URIError',
  'WeakMap',
  'WeakRef',
  'WeakSet',
  'Atomics',
  'JSON',
  'Math',
  'Reflect'
]

/**
 * A unique reference to a value owner by a "context object" and a
 * `Script` instance.
 */
export class Reference {
  /**
   * Predicate function to determine if a `value` is an internal or external
   * script reference value.
   * @param {amy} value
   * @return {boolean}
   */
  static isReference (value) {
    if (references.has(value)) {
      return true
    }

    if (value?.__vmScriptReference__ === true && typeof value?.id === 'string') {
      return true
    }

    return false
  }

  /**
   * The underlying reference ID.
   * @ignore
   * @type {string}
   */
  #id = null

  /**
   * The underling primitive type of the reference value.
   * @ignore
   * @type {'undefined'|'object'|'number'|'boolean'|'function'|'symbol'}
   */
  #type = 'undefined'

  /**
   * A strong reference to the underlying value.
   * @ignore
   * @type {any?}
   */
  #value = null

  /**
   * A weak reference to the underlying "context object", if available.
   * @ignore
   * @type {WeakRef?}
   */
  #context = null

  /**
   * A boolean value to indicate if the underlying reference value is an
   * intrinsic value.
   * @type {boolean}
   */
  #isIntrinsic = false

  /**
   * The intrinsic type this reference may be an instance of or directly refer to.
   * @type {function|object}
   */
  #intrinsicType = null

  /**
   * A boolean value to indicate if the underlying reference value is an
   * external reference value.
   * @type {boolean}
   */
  #isExternal = false

  /**
   * `Reference` class constructor.
   * @param {string} id
   * @param {any} value
   * @param {object=} [context]
   * @param {object=} [options]
   */
  constructor (id, value, context = null, options) {
    this.#id = id
    this.#type = value !== null ? typeof value : 'undefined'
    this.#value = value !== null && value !== undefined
      ? value
      : null

    this.#context = context !== null && context !== undefined
      ? new WeakRef(context)
      : null

    this.#intrinsicType = getIntrinsicType(this.#value)
    this.#isIntrinsic = isIntrinsic(this.#value)
    this.#isExternal = options?.external === true
  }

  /**
   * The unique id of the reference
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * The underling primitive type of the reference value.
   * @ignore
   * @type {'undefined'|'object'|'number'|'boolean'|'function'|'symbol'}
   */
  get type () {
    return this.#type
  }

  /**
   * The underlying value of the reference.
   * @type {any?}
   */
  get value () {
    return this.#value
  }

  /**
   * The name of the type.
   * @type {string?}
   */
  get name () {
    if (this.type === 'function') {
      return this.value.name
    }

    if (this.value && this.type === 'object') {
      const prototype = Reflect.getPrototypeOf(this.value)

      if (prototype?.constructor?.name) {
        return prototype.constructor.name
      }
    }

    return null
  }

  /**
   * The `Script` this value belongs to, if available.
   * @type {Script?}
   */
  get script () {
    return scripts.get(this.context) ?? null
  }

  /**
   * The "context object" this reference value belongs to.
   * @type {object?}
   */
  get context () {
    return this.#context?.deref?.() ?? null
  }

  /**
   * A boolean value to indicate if the underlying reference value is an
   * intrinsic value.
   * @type {boolean}
   */
  get isIntrinsic () {
    return this.#isIntrinsic
  }

  /**
   * A boolean value to indicate if the underlying reference value is an
   * external reference value.
   * @type {boolean}
   */
  get isExternal () {
    return this.#isExternal
  }

  /**
   * The intrinsic type this reference may be an instance of or directly refer to.
   * @type {function|object}
   */
  get intrinsicType () {
    return this.#intrinsicType
  }

  /**
   * Releases strongly held value and weak references
   * to the "context object".
   */
  release () {
    this.#value = null
    this.#context = null
  }

  /**
   * Converts this `Reference` to a JSON object.
   * @param {boolean=} [includeValue = false]
   */
  toJSON (includeValue = false) {
    const { isIntrinsic, name, type, id } = this
    const intrinsicType = getIntrinsicTypeString(this.intrinsicType)
    let { value } = this
    const json = {
      __vmScriptReference__: true,
      id,
      type,
      name,
      isIntrinsic,
      intrinsicType
    }

    if (includeValue) {
      if (
        value &&
        typeof value === 'object' &&
        Symbol.toStringTag in value
      ) {
        const tag = typeof value[Symbol.toStringTag] === 'function'
          ? value[Symbol.toStringTag]()
          : value[Symbol.toStringTag]

        if (tag === 'Module') {
          value = { ...value }
        }
      }

      json.value = value
    }

    return json
  }
}

/**
 * @typedef {{
 *  filename?: string,
 *  context?: object
 * }} ScriptOptions
 */

/**
 * A `Script` is a container for raw JavaScript to be executed in
 * a completely isolated virtual machine context, optionally with
 * user supplied context. Context objects references are not actually
 * shared, but instead provided to the script execution context using the
 * structured cloning algorithm used by the Message Channel API. Context
 * differences are computed and applied after execution so the user supplied
 * context object realizes context changes after script execution. All script
 * sources run in an "async" context so a "top level await" should work.
 */
export class Script extends EventTarget {
  #id = null
  #ready = null
  #source = null
  #context = null
  #filename = '<script>'

  /**
   * `Script` class constructor
   * @param {string} source
   * @param {ScriptOptions} [options]
   */
  constructor (source, options = null) {
    super()

    if (typeof source !== 'string') {
      throw new TypeError('Script source must be a string')
    }

    if (!source) {
      throw new TypeError('Script source cannot be empty')
    }

    this.#id = crypto.randomBytes(8).toString('base64')
    this.#source = source
    this.#context = options?.context ?? {}

    if (typeof options?.filename === 'string' && options.filename) {
      this.#filename = options.filename
    }

    gc.ref(this)

    this.#ready = getContextWindow()
      .then(getContextWorker())
      .catch((error) => {
        this.dispatchEvent(new ErrorEvent('error', { error }))
      })
  }

  /**
   * The script identifier.
   */
  get id () {
    return this.#id
  }

  /**
   * The source for this script.
   * @type {string}
   */
  get source () {
    return this.#source
  }

  /**
   * The filename for this script.
   * @type {string}
   */
  get filename () {
    return this.#filename
  }

  /**
   * A promise that resolves when the script is ready.
   * @type {Promise<Boolean>}
   */
  get ready () {
    return this.#ready
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.id],
      async handle (id) {
        const worker = await getContextWorker()
        worker.port.postMessage({ id, type: 'destroy' })
      }
    }
  }

  /**
   * Destroy the script execution context.
   * @return {Promise}
   */
  async destroy () {
    await this.ready
    const worker = await getContextWorker()
    worker.port.postMessage({ id: this.#id, type: 'destroy' })
  }

  /**
   * Run `source` JavaScript in given context. The script context execution
   * context is preserved until the `context` object that points to it is
   * garbage collected or there are no longer any references to it and its
   * associated `Script` instance.
   * @param {ScriptOptions=} [options]
   * @param {object=} [context]
   * @return {Promise<any>}
   */
  async runInContext (context, options = null) {
    await this.ready

    const contextReference = createContext(context ?? this.context)
    context = { ...(context ?? this.#context) }

    const filename = options?.filename || this.filename
    const worker = await getContextWorker()
    const source = options?.source ?? this.#source

    const transfer = []
    const nonce = crypto.randomBytes(8).toString('base64')
    const mode = options?.type ?? options?.mode ?? detectFunctionSourceType(source)
    const id = this.#id

    return await new Promise((resolve, reject) => {
      findMessageTransfers(transfer, context)
      filterNonTransferableValues(context)

      worker.port.postMessage({ type: 'client', id })
      worker.port.postMessage({
        type: 'script',
        filename,
        context,
        source,
        nonce,
        mode,
        id
      }, {
        transfer
      })

      worker.port.addEventListener('message', onMessage)

      function onMessage (event) {
        if (
          event.data?.id === id &&
          event.data?.nonce === nonce &&
          event.data?.type === 'result'
        ) {
          worker.port.removeEventListener('message', onMessage)
          if (event.data.context) {
            applyContextDifferences(contextReference, event.data.context, contextReference)
          }

          if (event.data.err) {
            reject(maybeMakeError(event.data.err))
          } else {
            const { data } = event
            const result = { data: data.data }
            // check if result data is an external reference
            const isReference = Reference.isReference(result.data)
            const name = isReference ? result.data.name : null

            if (name) {
              result[name] = result.data
              data[name] = data.data
              delete data.data
              delete result.data
            }

            applyContextDifferences(result, data, contextReference)

            if (name) {
              resolve(result[name])
            } else {
              resolve(result.data)
            }
          }
        }
      }
    })
  }

  /**
   * Run `source` JavaScript in new context. The script context is destroyed after
   * execution. This is typically a "one off" isolated run.
   * @param {ScriptOptions=} [options]
   * @param {object=} [context]
   * @return {Promise<any>}
   */
  async runInNewContext (context, options = null) {
    await this.ready

    const contextReference = context ?? this.context
    context = { ...context }

    const filename = options?.filename || this.filename
    const worker = await getContextWorker()
    const source = options?.source ?? this.#source

    const transfer = []
    const nonce = crypto.randomBytes(8).toString('base64')
    const mode = options?.type ?? options?.mode ?? detectFunctionSourceType(source)
    const id = crypto.randomBytes(8).toString('base64')

    const result = await new Promise((resolve, reject) => {
      findMessageTransfers(transfer, context)
      filterNonTransferableValues(context)

      worker.port.postMessage({ type: 'client', id })
      worker.port.postMessage({
        type: 'script',
        filename,
        context,
        source,
        nonce,
        mode,
        id
      }, {
        transfer
      })

      worker.port.addEventListener('message', onMessage)

      function onMessage (event) {
        if (
          event.data?.id === id &&
          event.data?.nonce === nonce &&
          event.data?.type === 'result'
        ) {
          worker.port.removeEventListener('message', onMessage)
          if (event.data.context) {
            applyContextDifferences(contextReference, event.data.context, contextReference)
          }

          if (event.data.err) {
            reject(maybeMakeError(event.data.err))
          } else {
            const { data } = event
            const result = { data: data.data }
            // check if result data is an external reference
            const isReference = Reference.isReference(result.data)
            const name = isReference ? result.data.name : null

            if (name) {
              result[name] = result.data
              data[name] = data.data
              delete data.data
              delete result.data
            }

            applyContextDifferences(result, data, contextReference)

            if (name) {
              resolve(result[name])
            } else {
              resolve(result.data)
            }
          }
        }
      }
    })

    worker.port.postMessage({ id, type: 'destroy' })
    return result
  }

  /**
   * Run `source` JavaScript in this current context (`globalThis`).
   * @param {ScriptOptions=} [options]
   * @return {Promise<any>}
   */
  async runInThisContext (options = null) {
    const filename = options?.filename || this.filename
    const fn = compileFunction(options?.source ?? this.#source, {
      async: true,
      filename
    })

    return await fn()
  }
}

/**
 * Gets the VM context window.
 * This function will create it if it does not already exist.
 * The current window will be used on Android or iOS platforms as there can
 * only be one window.
 * @return {Promise<import('./window.js').ApplicationWindow}
 */
export async function getContextWindow () {
  if (contextWindow) {
    await contextWindow.ready
    return contextWindow
  }

  const currentWindow = await application.getCurrentWindow()

  // just return the current window for android/ios as there can only ever be one
  if (
    os.platform() === 'ios' ||
    os.platform() === 'android' ||
    (os.platform() === 'win32' && !process.env.COREWEBVIEW2_22_AVAILABLE)
  ) {
    contextWindow = currentWindow

    if (!contextWindow.frame) {
      const frameId = `__${os.platform()}-vm-frame__`
      const existingFrame = globalThis.top.document.querySelector(
        `iframe[id="${frameId}"]`
      )

      if (existingFrame) {
        existingFrame.parentElement.removeChild(existingFrame)
      }

      const frame = globalThis.top.document.createElement('iframe')

      frame.setAttribute('sandbox', 'allow-same-origin allow-scripts')
      frame.src = VM_WINDOW_PATH
      frame.id = frameId

      Object.assign(frame.style, {
        display: 'none',
        height: 0,
        width: 0
      })

      const target = (
        globalThis.top.document.head ??
        globalThis.top.document.body ??
        globalThis.top.document
      )

      target.prepend(frame)
      contextWindow.frame = frame
      contextWindow.ready = new Promise((resolve, reject) => {
        frame.onload = resolve
        frame.onerror = (event) => {
          reject(new Error('Failed to load VM context window frame', {
            cause: event.error ?? event
          }))
        }
      })
    }

    await contextWindow.ready
    return contextWindow
  }

  const existingContextWindow = await application.getWindow(VM_WINDOW_INDEX, { max: false })
  const pendingContextWindow = (
    existingContextWindow ??
    application.createWindow({
      canExit: true,
      headless: !process.env.SOCKET_RUNTIME_VM_DEBUG,
      debug: Boolean(process.env.SOCKET_RUNTIME_VM_DEBUG),
      index: VM_WINDOW_INDEX,
      title: VM_WINDOW_TITLE,
      path: VM_WINDOW_PATH,
      config: {
        webview_watch_reload: false
      }
    })
  )

  const promises = []
  promises.push(pendingContextWindow)

  if (!existingContextWindow) {
    const eventName = `vm:${VM_WINDOW_INDEX}:ready`
    promises.push(new Promise((resolve) => {
      globalThis.addEventListener(eventName, resolve, { once: true })
    }))
  }

  const ready = Promise.all(promises)
  await ready
  contextWindow = await pendingContextWindow
  contextWindow.ready = ready
  return contextWindow
}

/**
 * Gets the `SharedWorker` that for the VM context.
 * @return {Promise<SharedWorker>}
 */
export async function getContextWorker () {
  if (contextWorker) {
    return await contextWorker.ready
  }

  // just return the current window for android/ios as there can only ever be one
  if (
    os.platform() === 'ios' ||
    os.platform() === 'android' ||
    (os.platform() === 'win32' && !process.env.COREWEBVIEW2_22_AVAILABLE)
  ) {
    if (globalThis.window && globalThis.top === globalThis.window) {
      // inside global top window
      contextWorker = new ContextWorkerInterface()
      contextWorker.ready = Promise.resolve(contextWorker)
      globals.register('vm.contextWorker', contextWorker)
    } else if (
      globalThis.window &&
      globalThis.top !== globalThis.window &&
      globalThis.location.pathname === new URL(VM_WINDOW_PATH).pathname
    ) {
      // inside realm frame
      contextWorker = new ContextWorkerInterfaceProxy(globalThis.top.__globals)
      contextWorker.ready = Promise.resolve(contextWorker)
    } else {
      throw new TypeError('Unable to determine VM context worker')
    }
  } else {
    contextWorker = new SharedWorker(`${globalThis.origin}/socket/vm/worker.js`, {
      type: 'module'
    })

    contextWorker.ready = new Promise((resolve, reject) => {
      contextWorker.addEventListener('error', (event) => {
        reject(new Error('Failed to initialize VM Context SharedWorker', {
          cause: event.error ?? event
        }))
      }, { once: true })

      contextWorker.port.addEventListener('message', (event) => {
        if (event.data === 'VM_SHARED_WORKER_ACK') {
          resolve(contextWorker)
        }
      }, { once: true })
    })
  }

  contextWorker.port.start()
  contextWorker.addEventListener('message', (event) => {
    if (event.data?.type === 'terminate-worker') {
      if (typeof contextWorker?.destroy === 'function') {
        contextWorker.destroy()
      }

      // unref
      contextWorker = null

      if (
        globalThis.window &&
        globalThis.top !== globalThis.window &&
        globalThis.location.pathname === new URL(VM_WINDOW_INDEX).pathname
      ) {
        globalThis.top.__globals.register('vm.contextWorker', contextWorker)
      }
    }
  })

  return await contextWorker.ready
}

/**
 * Terminates the VM script context window.
 * @ignore
 */
export async function terminateContextWindow () {
  const pendingContextWindow = getContextWindow()

  if (contextWindow.frame?.parentElement) {
    contextWindow.frame.parentElement.removeChild(contextWindow.frame)
  }

  contextWindow = null
  const currentContextWindow = await pendingContextWindow
  await currentContextWindow.close()

  const existingContextWindow = await application.getWindow(VM_WINDOW_INDEX, { max: false })

  if (existingContextWindow) {
    await existingContextWindow.close()
  }
}

/**
 * Terminates the VM script context worker.
 * @ignore
 */
export async function terminateContextWorker () {
  if (!contextWorker) {
    return
  }

  const worker = await getContextWorker()
  worker.port.postMessage({ type: 'terminate-worker' })
}

/**
 * Creates a prototype object of known global reserved intrinsics.
 * @ignore
 */
export function createIntrinsics (options) {
  const descriptors = Object.create(null)
  const propertyNames = Object.getOwnPropertyNames(globalThis)
  const propertySymbols = Object.getOwnPropertySymbols(globalThis)

  for (const property of propertyNames) {
    const intrinsic = Object.getOwnPropertyDescriptor(globalThis, property)
    const descriptor = Object.assign(Object.create(null), {
      configurable: options?.configurable === true,
      enumerable: true,
      value: intrinsic.value ?? globalThis[property] ?? undefined
    })

    descriptors[property] = descriptor
  }

  for (const symbol of propertySymbols) {
    descriptors[symbol] = {
      configurable: options?.configurable === true,
      enumberale: false,
      value: globalThis[symbol]
    }
  }

  return Object.create(null, descriptors)
}

/**
 * Returns `true` if value is an intrinsic, otherwise `false`.
 * @param {any} value
 * @return {boolean}
 */
export function isIntrinsic (value) {
  if (value === undefined) {
    return true
  }

  if (value === null) {
    return null
  }

  for (const key of RESERVED_GLOBAL_INTRINSICS) {
    const intrinsic = globalThis[key]
    if (intrinsic === value) {
      return true
    } else if (typeof intrinsic === 'function' && typeof value === 'object') {
      const prototype = Object.getPrototypeOf(value)
      if (prototype === intrinsic.prototype) {
        return true
      }
    }
  }

  return false
}

/**
 * Get the intrinsic type of a given `value`.
 * @param {any}
 * @return {function|object|null|undefined}
 */
export function getIntrinsicType (value) {
  if (value === undefined) {
    return undefined
  }

  if (value === null) {
    return null
  }

  for (const key of RESERVED_GLOBAL_INTRINSICS) {
    const intrinsic = globalThis[key]
    if (intrinsic === value) {
      return intrinsic
    } else if (typeof intrinsic === 'function' && typeof value === 'object') {
      const prototype = Object.getPrototypeOf(value)
      if (prototype === intrinsic.prototype) {
        return intrinsic
      }
    }
  }

  return undefined
}

/**
 * Get the intrinsic type string of a given `value`.
 * @param {any}
 * @return {string|null}
 */
export function getIntrinsicTypeString (value) {
  if (value === null) {
    return null
  }

  if (value === undefined) {
    return 'undefined'
  }

  for (const key of RESERVED_GLOBAL_INTRINSICS) {
    const intrinsic = globalThis[key]
    if (intrinsic === value) {
      return key
    } else if (typeof intrinsic === 'function' && typeof value === 'object') {
      const prototype = Object.getPrototypeOf(value)
      if (prototype === intrinsic.prototype) {
        return key
      }
    }
  }

  return null
}

/**
 * Creates a global proxy object for context execution.
 * @ignore
 * @param {object} context
 * @return {Proxy}
 */
export function createGlobalObject (context, options) {
  const existing = context && globals.get(context)

  if (existing) {
    return existing
  }

  const prototype = Object.getPrototypeOf(globalThis)
  const intrinsics = createIntrinsics(options)
  const descriptors = Object.getOwnPropertyDescriptors(intrinsics)
  const globalObject = Object.create(prototype, descriptors)

  const symbols = Object.getOwnPropertySymbols(intrinsics)
  const target = Object.create(null)

  // restore symbols
  for (const symbol of symbols) {
    try {
      Object.defineProperty(globalObject, symbol, intrinsics[symbol])
    } catch {}
  }

  const handler = {}
  const traps = {
    get (_, property, receiver) {
      if (property === 'console') {
        return console
      }

      if (context && property in context) {
        return Reflect.get(context, property)
      }

      if (property === 'top') {
        return null
      }

      if (property in globalObject) {
        return Reflect.get(globalObject, property)
      }
    },

    set (_, property, value) {
      if (context) {
        if (Reflect.isExtensible(context)) {
          Reflect.set(context, property, value)
          return true
        }

        return false
      }

      if (property === 'top') {
        return false
      }

      Reflect.set(globalObject, property, value)
      return true
    },

    getPrototypeOf (_) {
      if (context) {
        return Reflect.getPrototypeOf(context)
      }

      return prototype
    },

    setPrototypeOf (_, prototype) {
      return false
    },

    defineProperty (_, property, descriptor) {
      if (RESERVED_GLOBAL_INTRINSICS.includes(property)) {
        return true
      }

      if (context) {
        return (
          Reflect.defineProperty(context, property, descriptor) &&
          Reflect.getOwnPropertyDescriptor(context, property) !== undefined
        )
      }

      return (
        Reflect.defineProperty(globalObject, property, descriptor) &&
        Reflect.getOwnPropertyDescriptor(globalObject, property) !== undefined
      )
    },

    deleteProperty (_, property) {
      if (RESERVED_GLOBAL_INTRINSICS.includes(property)) {
        return false
      }

      if (context) {
        return Reflect.deleteProperty(context, property)
      }

      return Reflect.deleteProperty(globalObject, property)
    },

    getOwnPropertyDescriptor (_, property) {
      if (context) {
        const descriptor = Reflect.getOwnPropertyDescriptor(context, property)
        if (descriptor) {
          return descriptor
        }
      }

      if (property === 'top') {
        return { enumerable: false, configurable: false, value: null }
      }

      return Reflect.getOwnPropertyDescriptor(globalObject, property)
    },

    has (_, property) {
      if (context && Reflect.has(context, property)) {
        return true
      }

      if (property === 'top') {
        return true
      }

      return Reflect.has(globalObject, property)
    },

    isExtensible (_) {
      if (context) {
        return Reflect.isExtensible(context)
      }

      return true
    },

    ownKeys (_) {
      const keys = []
      if (context) {
        keys.push(...Reflect.ownKeys(context))
      }

      keys.push(...Reflect
        .ownKeys(globalObject)
        .filter((key) => {
          if (key === 'top') return false
          return true
        })
      )

      return Array.from(new Set(keys))
    },

    preventExtensions (_) {
      if (context) {
        Reflect.preventExtensions(context)
        return true
      }

      return false
    }
  }

  if (Array.isArray(options?.traps)) {
    for (const trap of options.traps) {
      if (typeof traps[trap] === 'function') {
        handler[trap] = traps[trap]
      }
    }
  } else if (options?.traps && typeof options?.traps === 'object') {
    for (const key in traps) {
      if (options.traps[key] !== false) {
        handler[key] = traps[key]
      }
    }
  } else {
    for (const key in traps) {
      handler[key] = traps[key]
    }
  }

  const proxy = new Proxy(target, handler)

  if (context) {
    globalObjects.set(context, proxy)
  }

  return proxy
}

/**
 * @ignore
 * @param {string} source
 * @return {boolean}
 */
export function detectFunctionSourceType (source) {
  if (detectESMSource(source)) {
    return 'module'
  }

  return 'classic'
}

/**
 * Compiles `source`  with `options` into a function.
 * @ignore
 * @param {string} source
 * @param {object=} [options]
 * @return {function}
 */
export function compileFunction (source, options = null) {
  source = convertSourceToString(source)

  options = { ...options }
  // detect source type naively
  if (!options?.type) {
    options.type = detectFunctionSourceType(source)
  }

  if (options?.type === 'module') {
    const hash = crypto.murmur3(source)
    let url = null

    if (blobURLCache.has(hash)) {
      url = blobURLCache.get(hash)
    } else {
      const blob = new Blob([source], { type: 'text/javascript' })
      url = URL.createObjectURL(blob)
      blobURLCache.set(hash, url)
    }

    const moduleSource = `
      const module = await import("${url}")
      const exports = {}

      if (module.default !== undefined) {
        exports.default = module.default
      }

      for (const key in module) {
        exports[key] = module[key]
      }

      return exports
    `

    return compileFunction(moduleSource, {
      ...options,
      type: 'classic',
      async: true,
      wrap: false
    })
  } else {
    const globalObject = (
      globalObjects.get(options?.context) ??
      createGlobalObject(options?.context)
    )

    const wrappedSource = options?.wrap === false
      ? source
      : wrapFunctionSource(source, options)

    const args = Array.from(options?.scope || []).concat(wrappedSource)
    const compiled = options?.async === true
      // eslint-disable-next-line
      ? new AsyncFunction(...args)
      // eslint-disable-next-line
      : new Function(...args)

    return compiled.bind(globalObject, globalObject)
  }
}

/**
 * Run `source` JavaScript in given context. The script context execution
 * context is preserved until the `context` object that points to it is
 * garbage collected or there are no longer any references to it and its
 * associated `Script` instance.
 * @param {string|object|function} source
 * @param {ScriptOptions=} [options]
 * @param {object=} [context]
 * @return {Promise<any>}
 */
export async function runInContext (source, options, context) {
  source = convertSourceToString(source)
  context = options?.context ?? context ?? sharedContext

  const script = scripts.get(context) ?? new Script(source, options)

  scripts.set(context, script)

  const result = await script.runInContext(context, {
    ...options,
    source
  })

  return result
}

/**
 * Run `source` JavaScript in new context. The script context is destroyed after
 * execution. This is typically a "one off" isolated run.
 * @param {string} source
 * @param {ScriptOptions=} [options]
 * @param {object=} [context]
 * @return {Promise<any>}
 */
export async function runInNewContext (source, options, context) {
  source = convertSourceToString(source)
  context = options?.context ?? context ?? {}
  const script = new Script(source, options)
  scripts.set(script.context, script)
  const result = await script.runInNewContext(context, options)
  await script.destroy()
  return result
}

/**
 * Run `source` JavaScript in this current context (`globalThis`).
 * @param {string} source
 * @param {ScriptOptions=} [options]
 * @return {Promise<any>}
 */
export async function runInThisContext (source, options) {
  source = convertSourceToString(source)

  const script = new Script(source, options)
  const result = await script.runInThisContext(options)
  await script.destroy()
  return result
}

/**
 * @ignore
 * @param {Reference} reference
 */
export function putReference (reference) {
  const { value } = reference
  if (
    value &&
    typeof value !== 'string' &&
    typeof value !== 'number' &&
    typeof value !== 'boolean'
  ) {
    references.set(value, reference)
  }

  references.index.set(reference.id, reference)
}

/**
 * Create a `Reference` for a `value` in a script `context`.
 * @param {any} value
 * @param {object} context
 * @param {object=} [options]
 * @return {Reference}
 */
export function createReference (value, context, options = null) {
  const id = crypto.randomBytes(8).toString('base64')
  const reference = new Reference(id, value, context, options)
  putReference(reference)
  return reference
}

/**
 * Get a script context by ID or values
 * @param {string|object|function} id
 * @return {Reference?}
 */
export function getReference (id) {
  return references.get(id) ?? references.index.get(id)
}

/**
 * Remove a script context reference by ID.
 * @param {string} id
 */
export function removeReference (id) {
  const reference = getReference(id)
  references.index.delete(id)
  if (reference) {
    references.delete(reference.value)
  }
}

/**
 * Get all transferable values in the `object` hierarchy.
 * @param {object} object
 * @return {object[]}
 */
export function getTransferables (object) {
  const transferables = []
  findMessageTransfers(transferables, object)
  return transferables
}

/**
 * @ignore
 * @param {object} object
 * @return {object}
 */
export function createContext (object) {
  if (isContext(object)) {
    return object
  } else if (object && typeof object === 'object') {
    contexts.set(object, kContextTag)
  }

  return object
}

/**
 * Returns `true` if `object` is a "context" object.
 * @param {object}
 * @return {boolean}
 */
export function isContext (object) {
  return contexts.has(object)
}

export default {
  createGlobalObject,
  compileFunction,
  createReference,
  getContextWindow,
  getContextWorker,
  getReference,
  getTransferables,
  putReference,
  Reference,
  removeReference,
  runInContext,
  runInNewContext,
  runInThisContext,
  Script,
  createContext,
  isContext
}
