/**
 * @module Async.AsyncContext
 *
 * Async Context for JavaScript based on the TC39 proposal.
 *
 * Example usage:
 * ```js
 * // `AsyncContext` is also globally available as `globalThis.AsyncContext`
 * import AsyncContext from 'socket:async/context'
 *
 * const var = new AsyncContext.Variable()
 * var.run('top', () => {
 *   console.log(var.get()) // 'top'
 *   queueMicrotask(() => {
 *     var.run('nested', () => {
 *       console.log(var.get()) // 'nested'
 *     })
 *   })
 * })
 * ```
 *
 * @see {@link https://tc39.es/proposal-async-context}
 * @see {@link https://github.com/tc39/proposal-async-context}
 */

/**
 * @template T
 * @typedef {{
 *   name?: string,
 *   defaultValue?: T
 * }} VariableOptions
 */

/**
 * @callback AnyFunc
 * @template T
 * @this T
 * @param {...any} args
 * @returns {any}
 */

/**
 * `FrozenRevert` holds a frozen Mapping that will be simply restored
 * when the revert is run.
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/fork.ts}
 */
export class FrozenRevert {
  /**
   * The (unchanged) mapping for this `FrozenRevert`.
   * @type {Mapping}
   */
  #mapping

  /**
   * `FrozenRevert` class constructor.
   * @param {Mapping} mapping
   */
  constructor (mapping) {
    this.#mapping = mapping
  }

  /**
   * Restores (unchaged) mapping from this `FrozenRevert`. This function is
   * called by `AsyncContext.Storage` when it reverts a current mapping to the
   * previous state before a "fork".
   * @param {Mapping=} [unused]
   * @return {Mapping}
   */
  restore (unused = null) {
    // eslint-disable-next-line
    void unused;
    return this.#mapping
  }
}

/**
 * Revert holds the state on how to revert a change to the
 * `AsyncContext.Storage` current `Mapping`
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/fork.ts}
 * @template T
 */
export class Revert {
  /**
   * @type {Variable<T>}
   */
  #key

  /**
   * @type {boolean}
   */
  #hasVariable = false

  /**
   * @type {T|undefined}
   */
  #previousVariable = undefined

  /**
   * `Revert` class constructor.
   * @param {Mapping} mapping
   * @param {Variable<T>} key
   */
  constructor (mapping, key) {
    this.#key = key
    this.#hasVariable = mapping.has(key)
    this.#previousVariable = mapping.get(key)
  }

  /**
   * @type {T|undefined}
   */
  get previousVariable () {
    return this.#previousVariable
  }

  /**
   * Restores a mapping from this `Revert`. This function is called by
   * `AsyncContext.Storage` when it reverts a current mapping to the
   * previous state before a "fork".
   * @param {Mapping} current
   * @return {Mapping}
   */
  restore (current) {
    if (this.#hasVariable) {
      return current.set(this.#key, this.#previousVariable)
    }

    return current.delete(this.#key)
  }
}

/**
 * A container for all `AsyncContext.Variable` instances and snapshot state.
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/mapping.ts}
 */
export class Mapping {
  /**
   * The internal mapping data.
   * @type {Map<Variable<any>, any>}
   */
  #data = null

  /**
   * `true` if the `Mapping` is frozen, otherwise `false`.
   * @type {boolean}
   */
  #frozen = false

  /**
   * `Mapping` class constructor.
   * @param {Map<Variable<any>, any>} data
   */
  constructor (data) {
    this.#data = data
  }

  /**
   * Freezes the `Mapping` preventing `AsyncContext.Variable` modifications with
   * `set()` and `delete()`.
   */
  freeze () {
    this.#frozen = true
  }

  /**
   * Returns `true` if the `Mapping` is frozen, otherwise `false`.
   * @return {boolean}
   */
  isFrozen () {
    return this.#frozen
  }

  /**
   * Optionally returns a new `Mapping` if the current one is "frozen",
   * otherwise it just returns the current instance.
   * @return {Mapping}
   */
  fork () {
    if (this.#frozen) {
      return new this.constructor(new Map(this.#data))
    }

    return this
  }

  /**
   * Returns `true` if the `Mapping` has a `AsyncContext.Variable` at `key`,
   * otherwise `false.
   * @template T
   * @param {Variable<T>} key
   * @return {boolean}
   */
  has (key) {
    return this.#data.has(key)
  }

  /**
   * Gets an `AsyncContext.Variable` value at `key`. If not set, this function
   * returns `undefined`.
   * @template T
   * @param {Variable<T>} key
   * @return {boolean}
   */
  get (key) {
    return this.#data.get(key)
  }

  /**
   * Sets an `AsyncContext.Variable` value at `key`. If the `Mapping` is frozen,
   * then a "forked" (new) instance with the value set on it is returned,
   * otherwise the current instance.
   * @template T
   * @param {Variable<T>} key
   * @param {T} value
   * @return {Mapping}
   */
  set (key, value) {
    const mapping = this.fork()
    mapping.#data.set(key, value)
    return mapping
  }

  /**
   * Delete  an `AsyncContext.Variable` value at `key`.
   * If the `Mapping` is frozen, then a "forked" (new) instance is returned,
   * otherwise the current instance.
   * @template T
   * @param {Variable<T>} key
   * @param {T} value
   * @return {Mapping}
   */
  delete (key) {
    const mapping = this.fork()
    mapping.#data.delete(key)
    return mapping
  }
}

/**
 * A container of all `AsyncContext.Variable` data.
 * @ignore
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/src/storage.ts}
 */
export class Storage {
  /**
   * The current `Mapping` for this `AsyncContext`.
   * @type {Mapping}
   */
  static #current = new Mapping(new Map())

  /**
   * Returns `true` if the current `Mapping` has a
   * `AsyncContext.Variable` at `key`,
   * otherwise `false.
   * @template T
   * @param {Variable<T>} key
   * @return {boolean}
   */
  static has (key) {
    return this.#current.has(key)
  }

  /**
   * Gets an `AsyncContext.Variable` value at `key` for the current `Mapping`.
   * If not set, this function returns `undefined`.
   * @template T
   * @param {Variable<T>} key
   * @return {T|undefined}
   */
  static get (key) {
    return this.#current.get(key)
  }

  /**
   * Set updates the `AsyncContext.Variable` with a new value and returns a
   * revert action that allows the modification to be reversed in the future.
   * @template T
   * @param {Variable<T>} key
   * @param {T} value
   * @return {Revert<T>|FrozenRevert}
   */
  static set (key, value) {
    const revert = this.#current.isFrozen()
      ? new FrozenRevert(this.#current)
      : /** @type {Revert<T>} */ (new Revert(this.#current, key))
    this.#current = this.#current.set(key, value)
    return revert
  }

  /**
   * "Freezes" the current storage `Mapping`, and returns a new `FrozenRevert`
   * or `Revert` which can restore the storage state to the state at
   * the time of the snapshot.
   * @return {FrozenRevert}
   */
  static snapshot () {
    this.#current.freeze()
    return new FrozenRevert(this.#current)
  }

  /**
   * Restores the storage `Mapping` state to state at the time the
   * "revert" (`FrozenRevert` or `Revert`) was created.
   * @template T
   * @param {Revert<T>|FrozenRevert} revert
   */
  static restore (revert) {
    this.#current = revert.restore(this.#current)
  }

  /**
   * Switches storage `Mapping` state to the state at the time of a
   * "snapshot".
   * @param {FrozenRevert} snapshot
   * @return {FrozenRevert}
   */
  static switch (snapshot) {
    const previous = this.#current
    this.#current = snapshot.restore(previous)
    return new FrozenRevert(previous)
  }
}

/**
 * `AsyncContext.Variable` is a container for a value that is associated with
 * the current execution flow. The value is propagated through async execution
 * flows, and can be snapshot and restored with Snapshot.
 * @template T
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextvariable}
 */
export class Variable {
  /**
   * The name of this async context variable.
   * @ignore
   * @type {string}
   */
  #name = ''

  /**
   * The default value of this async context variable.
   * @ignore
   * @type {T|undefined}
   */
  #defaultValue = undefined

  /**
   * @type {FrozenRevert|Revert<T>|undefined}
   */
  #revert = undefined

  /**
   * `Variable` class constructor.
   * @param {VariableOptions<T>=} [options]
   */
  constructor (options = null) {
    if (options?.name && typeof options?.name === 'string') {
      this.#name = options.name
    }

    this.#defaultValue = options?.defaultValue
  }

  /**
   * @ignore
   */
  get defaultValue () { return this.#defaultValue }
  set defaultValue (defaultValue) {
    this.#defaultValue = defaultValue
  }

  /**
   * @ignore
   */
  get revert () {
    return this.#revert
  }

  /**
   * The name of this async context variable.
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * Executes a function `fn` with specified arguments,
   * setting a new value to the current context before the call,
   * and ensuring the environment is reverted back afterwards.
   * The function allows for the modification of a specific context's
   * state in a controlled manner, ensuring that any changes can be undone.
   * @template T, F extends AnyFunc<null>
   * @param {T} value
   * @param {F} fn
   * @param {...Parameters<F>} args
   * @returns {ReturnType<F>}
   */
  run (value, fn, ...args) {
    const revert = Storage.set(this, value)
    this.#revert = revert
    try {
      return Reflect.apply(fn, null, args)
    } finally {
      Storage.restore(revert)
      if (this.#revert === revert) {
        this.#revert = null
      }
    }
  }

  /**
   * Get the `AsyncContext.Variable` value.
   * @template T
   * @return {T|undefined}
   */
  get () {
    return Storage.has(this) ? Storage.get(this) : this.#defaultValue
  }
}

/**
 * `AsyncContext.Snapshot` allows you to opaquely capture the current values of
 * all `AsyncContext.Variable` instances and execute a function at a later time
 * as if those values were still the current values (a snapshot and restore).
 * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshot}
 */
export class Snapshot {
  #snapshot = Storage.snapshot()

  /**
   * Wraps a given function `fn` with additional logic to take a snapshot of
   * `Storage` before invoking `fn`. Returns a new function with the same
   * signature as `fn` that when called, will invoke `fn` with the current
   * `this` context and provided arguments, after restoring the `Storage`
   * snapshot.
   *
   * `AsyncContext.Snapshot.wrap` is a helper which captures the current values
   * of all Variables and returns a wrapped function. When invoked, this
   * wrapped function restores the state of all Variables and executes the
   * inner function.
   *
   * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshotwrap}
   *
   * @template F
   * @param {F} fn
   * @returns {F}
   */
  static wrap (fn) {
    const name = fn.name || 'anonymous'
    const snapshot = Storage.snapshot()
    const container = {
      /**
       * @this {ThisType<F>}
       * @param {...any} args
       * @returns {ReturnType<F>}
       */
      [name] (...args) {
        return run(fn, this, args, snapshot)
      }
    }

    return /** @type {F} */ (container[name])
  }

  /**
   * Runs the given function `fn` with arguments `args`, using a `null`
   * context and the current snapshot.
   *
   * @template F extends AnyFunc<null>
   * @param {F} fn
   * @param {...Parameters<F>} args
   * @returns {ReturnType<F>}
   */
  run (fn, ...args) {
    return run(fn, null, args, this.#snapshot)
  }
}

/**
 * Runs the given function `fn` with the provided `context` and `args`,
 * ensuring the environment is set to a specific `snapshot` before execution,
 * and reverted back afterwards.
 *
 * @template F extends AnyFunc<any>
 * @param {F} fn
 * @param {ThisType<F>} context
 * @param {any[]} args
 * @param {FrozenRevert} snapshot
 * @returns {ReturnType<F>}
 */
function run (fn, context, args, snapshot) {
  const revert = Storage.switch(snapshot)

  try {
    return Reflect.apply(fn, context, args)
  } finally {
    Storage.restore(revert)
  }
}

/**
 * `AsyncContext` container.
 */
export class AsyncContext {
  /**
   * `AsyncContext.Variable` is a container for a value that is associated with
   * the current execution flow. The value is propagated through async execution
   * flows, and can be snapshot and restored with Snapshot.
   * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextvariable}
   * @type {typeof Variable}
   */
  static Variable = Variable

  /**
   * `AsyncContext.Snapshot` allows you to opaquely capture the current values of
   * all `AsyncContext.Variable` instances and execute a function at a later time
   * as if those values were still the current values (a snapshot and restore).
   * @see {@link https://github.com/tc39/proposal-async-context/blob/master/README.md#asynccontextsnapshot}
   * @type {typeof Snapshot}
   */
  static Snapshot = Snapshot
}

export default AsyncContext
