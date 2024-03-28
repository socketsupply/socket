import { createHook } from '../async/hooks.js'

let isAsyncContext = false
const asyncContexts = new Set([
  'Promise',
  'Timeout',
  'Interval',
  'Immediate',
  'Microtask'
])

const hook = createHook({
  before (asyncId, type) {
    if (asyncContexts.has(type)) {
      isAsyncContext = true
    }
  },

  after (asyncId, type) {
    if (asyncContexts.has(type)) {
      isAsyncContext = false
    }
  }
})

hook.enable()

export class CallSite {
  static PromiseElementIndexSymbol = Symbol.for('socket.runtime.CallSite.PromiseElementIndex')
  static PromiseAllSymbol = Symbol.for('socket.runtime.CallSite.PromiseAll')
  static PromiseAnySymbol = Symbol.for('socket.runtime.CallSite.PromiseAny')

  #error = null

  constructor (error) {
    this.#error = error
  }

  get error () {
    return this.#error
  }

  getThis () {
  }

  getTypeName () {
  }

  getFunction () {
  }

  getFunctionName () {
    const stack = this.#error.split('\n')
    const parts = stack.split('@')
    if (!parts[0]) {
      return '<anonymous>'
    }

    return parts[0]
  }

  getFileName () {
    if (URL.canParse(this.#error.sourceURL)) {
      const url = new URL(this.#error.sourceURL)
      return url.href
    }

    return 'unknown location'
  }

  getLineNumber () {
    return this.#error.line
  }

  getColumnNumber () {
    return this.#error.column
  }

  getEvalOrigin () {
  }

  isTopLevel () {
  }

  isEval () {
  }

  isNative () {
    return false
  }

  isConstructor () {
  }

  isAsync () {
    return isAsyncContext
  }

  isPromiseAll () {
    return this.#error
  }
}

export default CallSite
