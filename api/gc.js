import { FinalizationRegistryCallbackError } from './errors.js'
import symbols from './internal/symbols.js'

if (typeof FinalizationRegistry === 'undefined') {
  console.warn(
    'FinalizationRegistry is not implemented in this environment. ' +
    'gc.ref() will have no effect.'
  )
}

export const finalizers = new WeakMap()
export const kFinalizer = Symbol.for('socket.runtime.gc.finalizer')
export const finalizer = kFinalizer
/**
 * @type {Set<WeakRef>}
 */
export const pool = new Set()

/**
 * Static registry for objects to clean up underlying resources when they
 * are gc'd by the environment. There is no guarantee that the `finalizer()`
 * is called at any time.
 */
export const registry = new FinalizationRegistry(finalizationRegistryCallback)

/**
 * Default exports which also acts a retained value to persist bound
 * `Finalizer#handle()` functions from being gc'd before the
 * `FinalizationRegistry` callback is called because `heldValue` must be
 * strongly held (retained) in order for the callback to be called.
 */
export const gc = Object.freeze(Object.create(null, Object.getOwnPropertyDescriptors({
  ref,
  pool,
  unref,
  retain,
  release,
  registry,
  finalize,
  finalizer,
  finalizers,

  get refs () { return pool.size }
})))

// `gc` is also the default export
export default gc

/**
 * Internal `FinalizationRegistry` callback.
 * @private
 * @param {Finalizer} finalizer
 */
async function finalizationRegistryCallback (finalizer) {
  if (!finalizer) return // potentially called when finalizer is already gone

  if (typeof finalizer.handle === 'function') {
    try {
      await finalizer.handle(...finalizer.args)
    } catch (e) {
      const err = new FinalizationRegistryCallbackError(e.message, {
        cause: e
      })

      // @ts-ignore
      if (typeof Error.captureStackTrace === 'function') {
        // @ts-ignore
        Error.captureStackTrace(err, finalizationRegistryCallback)
      }

      console.warn(err.name, err.message, err.stack, err.cause)
    }
  }

  for (const weakRef of pool) {
    if (weakRef instanceof WeakRef) {
      const ref = weakRef.deref()
      if (ref && ref !== finalizer) {
        continue
      }
    }

    pool.delete(weakRef)
  }

  finalizer = undefined
}

/**
 * A container for strongly (retain) referenced finalizer function
 * with arguments weakly referenced to an object that will be
 * garbage collected.
 */
export class Finalizer {
  args = []
  handle = null

  /**
   * Creates a `Finalizer` from input.
   */
  static from (handler) {
    if (typeof handler === 'function') {
      return new this([], handler)
    }

    let { handle, args } = handler

    if (typeof handle !== 'function') {
      handle = () => {}
    }

    if (!Array.isArray(args)) {
      args = []
    }

    return new this(args, handle)
  }

  /**
   * `Finalizer` class constructor.
   * @private
   * @param {array} args
   * @param {function} handle
   */
  constructor (args, handle) {
    this.args = args
    this.handle = handle.bind(gc)
  }
}

/**
 * Track `object` ref to call `Symbol.for('socket.runtime.gc.finalize')` method when
 * environment garbage collects object.
 * @param {object} object
 * @return {boolean}
 */
export async function ref (object, ...args) {
  if (
    object && (
      typeof object[kFinalizer] === 'function' ||
      typeof object[symbols.dispose] === 'function'
    )
  ) {
    const finalizer = Finalizer.from(
      await (object[kFinalizer] || object[symbols.dispose]).call(object, ...args)
    )

    const weakRef = new WeakRef(finalizer)

    finalizers.set(object, weakRef)
    pool.add(weakRef)

    registry.register(object, finalizer, object)
  }

  return finalizers.has(object)
}

/**
 * Stop tracking `object` ref to call `Symbol.for('socket.runtime.gc.finalize')` method when
 * environment garbage collects object.
 * @param {object} object
 * @return {boolean}
 */
export function unref (object) {
  if (!object || typeof object !== 'object') {
    return false
  }

  if (
    finalizers.has(object) && (
      typeof object[kFinalizer] === 'function' ||
      typeof object[symbols.dispose] === 'function'
    )
  ) {
    const weakRef = finalizers.get(object)

    if (weakRef) {
      pool.delete(weakRef)
    }

    finalizers.delete(object)
    registry.unregister(object)
    return true
  }

  return false
}

/**
 * An alias for `unref()`
 * @param {object} object}
 * @return {boolean}
 */
export function retain (object) {
  return unref(object)
}

/**
 * Call finalize on `object` for `gc.finalizer` implementation.
 * @param {object} object]
 * @return {Promise<boolean>}
 */
export async function finalize (object, ...args) {
  const finalizer = finalizers.get(object)?.deref()

  registry.unregister(object)

  try {
    if (finalizer instanceof Finalizer && await unref(object)) {
      await finalizationRegistryCallback(finalizer)
    } else {
      const finalizer = Finalizer.from(
        await (object[kFinalizer] || object[symbols.dispose]).call(object, ...args)
      )

      await finalizationRegistryCallback(finalizer)
    }
    return true
  } catch (err) {
    return false
  }
}

/**
 * Calls all pending finalization handlers forcefully. This function
 * may have unintended consequences as objects be considered finalized
 * and still strongly held (retained) somewhere.
 */
export async function release () {
  for (const weakRef of pool) {
    // @ts-ignore
    await finalizationRegistryCallback(weakRef?.deref?.())
    pool.delete(weakRef)
  }
}
