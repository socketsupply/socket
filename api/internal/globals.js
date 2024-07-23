import Promise from './promise.js'

/**
 * Symbolic global registry
 * @ignore
 */
export class GlobalsRegistry {
  get global () {
    return globalThis ?? this
  }

  symbol (name) {
    return Symbol.for(`socket.runtime.global.${name}`)
  }

  register (name, value) {
    this.global[this.symbol(name)] = value
    return value
  }

  get (name) {
    return this.global[this.symbol(name)] ?? null
  }
}

const registry = (
  globalThis.top?.__globals ??
  globalThis.__globals ??
  new GlobalsRegistry()
)

const RuntimeReadyPromiseResolvers = Promise.withResolvers()
registry.register('RuntimeReadyPromiseResolvers', RuntimeReadyPromiseResolvers)
registry.register('RuntimeReadyPromise', RuntimeReadyPromiseResolvers.promise)

/**
 * Gets a runtime global value by name.
 * @ignore
 * @param {string} name
 * @return {any|null}
 */
export function get (name) {
  return registry.get(name) ?? null
}

export default registry
