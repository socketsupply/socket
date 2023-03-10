/**
 * Symbolic global registry
 * @ignore
 */
/* eslint-disable-next-line new-parens */
const registry = new class GlobalsRegistry {
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

/**
 * Gets a global by name.
 * @ignore
 */
export function get (name) {
  return registry.get(name)
}

export default registry
