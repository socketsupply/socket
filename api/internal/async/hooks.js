import { wrap as asyncWrap } from '../../async/wrap.js'
import { toProperCase } from '../../util.js'
import { Variable } from '../../async/context.js'
import gc from '../../gc.js'

let currentAsyncResourceId = 1

/**
 * The default top level async resource ID
 * @type {number}
 */
export const TOP_LEVEL_ASYNC_RESOURCE_ID = 1

/**
 * The internal async hook state.
 */
export const state = {
  defaultExecutionAsyncId: -1
}

/**
 * The current async hooks enabled.
 */
export const hooks = {
  init: [],
  before: [],
  after: [],
  destroy: [],
  promiseResolve: []
}

/**
 * A base class for the `AsyncResource` class or other higher level async
 * resource classes.
 */
export class CoreAsyncResource {
  #type = null
  #destroyed = false
  #asyncId = getNextAsyncResourceId()
  #triggerAsyncId = getDefaultExecutionAsyncId()
  #requireManualDestroy = false

  /**
   * `CoreAsyncResource` class constructor.
   * @param {string} type
   * @param {object|number=} [options]
   */
  constructor (type, options = null) {
    if (!type || typeof type !== 'string') {
      throw new TypeError(
        `Expecting 'type' to be a string. Received: ${type}`
      )
    }

    if (typeof options === 'number') {
      options = { triggerAsyncId: options }
    }

    this.#type = type

    if (Number.isFinite(options?.triggerAsyncId) && options.triggerAsyncId > 0) {
      this.#triggerAsyncId = options.triggerAsyncId
    } else if (options?.triggerAsyncId !== undefined) {
      throw new TypeError(
        // eslint-disable-next-line
        `Expecting 'options.triggerAsyncId' to be a positive number.` +
        `Received: ${options.triggerAsyncId}`
      )
    }

    if (typeof options?.requireManualDestroy === 'boolean') {
      this.#requireManualDestroy = options.requireManualDestroy
    } else if (options?.requireManualDestroy !== undefined) {
      throw new TypeError(
        // eslint-disable-next-line
        `Expecting 'options.requireManualDestroy' to be a boolean.` +
        `Received: ${options.requireManualDestroy}`
      )
    }

    dispatch('init', this.asyncId(), this.type, this.triggerAsyncId(), this)

    if (!this.#requireManualDestroy) {
      gc.ref(this)
    }
  }

  /**
   * The `CoreAsyncResource` type.
   * @type {string}
   */
  get type () {
    return this.#type
  }

  /**
   * `true` if the `CoreAsyncResource` was destroyed, otherwise `false`. This
   * value is only set to `true` if `emitDestroy()` was called, likely from
   * destroying the resource manually.
   * @type {boolean}
   */
  get destroyed () {
    return this.#destroyed
  }

  /**
   * The unique async resource ID.
   * @return {number}
   */
  asyncId () {
    return this.#asyncId
  }

  /**
   * The trigger async resource ID.
   * @return {number}
   */
  triggerAsyncId () {
    return this.#triggerAsyncId
  }

  /**
   * Manually emits destroy hook for the resource.
   * @return {CoreAsyncResource}
   */
  emitDestroy () {
    if (!this.#destroyed) {
      this.#destroyed = true
    }

    dispatch('destroy', this.asyncId(), this.type, this.triggerAsyncId(), this)
    return this
  }

  /**
   * Binds function `fn` with an optional this `thisArg` binding to run
   * in the execution context of this `CoreAsyncResource`.
   * @param {function} fn
   * @param {object=} [thisArg]
   * @return {function}
   */
  bind (fn, thisArg = undefined) {
    const resource = this
    let binding = null

    if (thisArg === undefined) {
      binding = function (...args) {
        args.unshift(fn, this)
        return Reflect.apply(resource.runInAsyncScope, resource, args)
      }
    } else {
      binding = resource.runInAsyncScope.bind(this, fn, thisArg)
    }

    Object.defineProperty(binding, 'length', {
      __proto__: null,
      configurable: true,
      enumerable: false,
      writable: false,
      value: fn.length
    })

    return binding
  }

  /**
   * Runs function `fn` in the execution context of this `CoreAsyncResource`.
   * @param {function} fn
   * @param {object=} [thisArg]
   * @param {...any} [args]
   * @return {any}
   */
  runInAsyncScope (fn, thisArg, ...args) {
    dispatch('before', this.asyncId(), this.type, this.triggerAsyncId(), this)
    try {
      return asyncContextVariable.run(this, fn.bind(thisArg), ...args)
    } finally {
      dispatch('after', this.asyncId(), this.type, this.triggerAsyncId(), this)
    }
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.asyncId(), this.type, this.triggerAsyncId()],
      handle (asyncId, type, triggerAsyncId) {
        dispatch('destroy', asyncId, type, triggerAsyncId)
      }
    }
  }
}

export class TopLevelAsyncResource extends CoreAsyncResource {}

export function dispatch (hook, asyncId, type, triggerAsyncId, resource) {
  if (hook in hooks) {
    for (const callback of hooks[hook]) {
      callback(asyncId, type, triggerAsyncId, resource)
    }
  }
}

export function getNextAsyncResourceId () {
  return ++currentAsyncResourceId
}

export function executionAsyncResource () {
  return asyncContextVariable.get()
}

export function executionAsyncId () {
  const resource = executionAsyncResource()
  if (resource && typeof resource.asyncId === 'function') {
    return resource.asyncId()
  }

  return TOP_LEVEL_ASYNC_RESOURCE_ID
}

export function triggerAsyncId () {
  const revert = asyncContextVariable.revert
  if (revert?.previousVariable) {
    const resource = revert.previousVariable.get()
    if (resource && typeof resource.asyncId === 'function') {
      return resource.asyncId()
    }
  }

  return TOP_LEVEL_ASYNC_RESOURCE_ID
}

export function getDefaultExecutionAsyncId () {
  if (state.defaultExecutionAsyncId < 0) {
    return executionAsyncId()
  }

  return state.defaultExecutionAsyncId
}

export function wrap (
  callback,
  type,
  asyncId = getNextAsyncResourceId(),
  triggerAsyncId = getDefaultExecutionAsyncId(),
  resource = undefined
) {
  dispatch('init', asyncId, type, triggerAsyncId, resource)
  callback = asyncWrap(callback)
  return function (...args) {
    dispatch('before', asyncId, type, triggerAsyncId)
    try {
      return (resource || topLevelAsyncResource).runInAsyncScope(() => {
        // eslint-disable-next-line
        return callback(...args)
      })
    } finally {
      dispatch('after', asyncId, type, triggerAsyncId)
    }
  }
}

export function getTopLevelAsyncResourceName () {
  if (globalThis.__args?.client) {
    const { type, frameType } = globalThis.__args.client
    return (
      frameType.replace('none', '').split('-').filter(Boolean).map(toProperCase).join('') +
      toProperCase(type)
    )
  }

  return 'TopLevel'
}

export const asyncContextVariable = new Variable({ name: 'internal/async/hooks' })
export const topLevelAsyncResource = new TopLevelAsyncResource(
  getTopLevelAsyncResourceName()
)

asyncContextVariable.defaultValue = topLevelAsyncResource

export default hooks
