import * as exports from './errors.js'

export default exports

export const ABORT_ERR = 20
export const ENCODING_ERR = 32
export const INVALID_ACCESS_ERR = 15
export const INDEX_SIZE_ERR = 1
export const NETWORK_ERR = 19
export const NOT_ALLOWED_ERR = 31
export const NOT_FOUND_ERR = 8
export const NOT_SUPPORTED_ERR = 9
export const OPERATION_ERR = 30
export const TIMEOUT_ERR = 23

/**
 * An `AbortError` is an error type thrown in an `onabort()` level 0
 * event handler on an `AbortSignal` instance.
 */
export class AbortError extends Error {
  /**
   * The code given to an `ABORT_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return ABORT_ERR }

  /**
   * `AbortError` class constructor.
   * @param {AbortSignal|string} reasonOrSignal
   * @param {AbortSignal=} [signal]
   */
  constructor (reason, signal, ...args) {
    if (reason?.reason) {
      signal = reason
      reason = signal.reason
    }

    super(reason || signal?.reason || 'The operation was aborted', ...args)

    this.signal = signal || null

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, AbortError)
    }
  }

  get name () {
    return 'AbortError'
  }

  get code () {
    return 'ABORT_ERR'
  }
}

/**
 * An `BadRequestError` is an error type thrown in an `onabort()` level 0
 * event handler on an `BadRequestSignal` instance.
 */
export class BadRequestError extends Error {
  /**
   * The default code given to a `BadRequestError`
   */
  static get code () { return 0 }

  /**
   * `BadRequestError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, BadRequestError)
    }
  }

  get name () {
    return 'BadRequestError'
  }

  get code () {
    return 'BAD_REQUEST_ERR'
  }
}

/**
 * An `EncodingError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class EncodingError extends Error {
  /**
   * The code given to an `ENCODING_ERR` `DOMException`.
   */
  static get code () { return ENCODING_ERR }

  /**
   * `EncodingError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, EncodingError)
    }
  }

  get name () {
    return 'EncodingError'
  }

  get code () {
    return 'ENCODING_ERR'
  }
}

/**
 * An `FinalizationRegistryCallbackError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class FinalizationRegistryCallbackError extends Error {
  /**
   * The default code given to an `FinalizationRegistryCallbackError`
   */
  static get code () { return 0 }

  /**
   * `FinalizationRegistryCallbackError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, FinalizationRegistryCallbackError)
    }
  }

  get name () {
    return 'FinalizationRegistryCallbackError'
  }

  get code () {
    return 'FINALIZATION_REGISTRY_CALLBACK_ERR'
  }
}

/**
 * An `IllegalConstructorError` is an error type thrown when a constructor is
 * called for a class constructor when it shouldn't be.
 */
export class IllegalConstructorError extends TypeError {
  /**
   * The default code given to an `IllegalConstructorError`
   */
  static get code () { return 0 }

  /**
   * `IllegalConstructorError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, IllegalConstructorError)
    }
  }

  get name () {
    return 'IllegalConstructorError'
  }

  get code () {
    return 'ILLEGAL_CONSTRUCTOR_ERR'
  }
}

/**
 * An `IndexSizeError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class IndexSizeError extends Error {
  /**
   * The code given to an `NOT_FOUND_ERR` `DOMException`
   */
  static get code () { return INDEX_SIZE_ERR }

  /**
   * `IndexSizeError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, IndexSizeError)
    }
  }

  get name () {
    return 'IndexSizeError'
  }

  get code () {
    return 'INDEX_SIZE_ERR'
  }
}

export const kInternalErrorCode = Symbol.for('InternalError.code')

/**
 * An `InternalError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class InternalError extends Error {
  /**
   * The default code given to an `InternalError`
   */
  static get code () { return 0 }

  /**
   * `InternalError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, code, ...args) {
    super(message, code, ...args)

    if (code) {
      this[kInternalErrorCode] = code
    }

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, InternalError)
    }
  }

  get name () {
    return 'InternalError'
  }

  get code () {
    return this[kInternalErrorCode] || 'INTERNAL_ERR'
  }

  set code (code) {
    this[kInternalErrorCode] = code
  }
}

/**
 * An `InvalidAccessError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class InvalidAccessError extends Error {
  /**
   * The code given to an `INVALID_ACCESS_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return INVALID_ACCESS_ERR }

  /**
   * `InvalidAccessError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, InvalidAccessError)
    }
  }

  get name () {
    return 'InvalidAccessError'
  }

  get code () {
    return 'INVALID_ACCESS_ERR'
  }
}

/**
 * An `NetworkError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class NetworkError extends Error {
  /**
   * The code given to an `NETWORK_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return NETWORK_ERR }

  /**
   * `NetworkError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    this.name = 'NetworkError'

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, NetworkError)
    }
  }

  get name () {
    return 'NetworkError'
  }

  get code () {
    return 'NETWORK_ERR'
  }
}

/**
 * An `NotAllowedError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class NotAllowedError extends Error {
  /**
   * The code given to an `NOT_ALLOWED_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return NOT_ALLOWED_ERR }

  /**
   * `NotAllowedError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, NotAllowedError)
    }
  }

  get name () {
    return 'NotAllowedError'
  }

  get code () {
    return 'NOT_ALLOWED_ERR'
  }
}

/**
 * An `NotFoundError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class NotFoundError extends Error {
  /**
   * The code given to an `NOT_FOUND_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return NOT_FOUND_ERR }

  /**
   * `NotFoundError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, NotFoundError)
    }
  }

  get name () {
    return 'NotFoundError'
  }

  get code () {
    return 'NOT_FOUND_ERR'
  }
}

/**
 * An `NotSupportedError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class NotSupportedError extends Error {
  /**
   * The code given to an `NOT_SUPPORTED_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return NOT_SUPPORTED_ERR }

  /**
   * `NotSupportedError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, NotSupportedError)
    }
  }

  get name () {
    return 'NotSupportedError'
  }

  get code () {
    return 'NOT_SUPPORTED_ERR'
  }
}

/**
 * An `ModuleNotFoundError` is an error type thrown when an imported or
 * required module is not found.
 */
export class ModuleNotFoundError extends NotFoundError {
  /**
   * `ModuleNotFoundError` class constructor.
   * @param {string} message
   */
  constructor (message, requireStack) {
    super(message)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, ModuleNotFoundError)
    }

    this.requireStack = requireStack || []
  }

  get name () {
    return 'ModuleNotFoundError'
  }

  get code () {
    return 'MODULE_NOT_FOUND'
  }
}

/**
 * An `OperationError` is an error type thrown when an internal exception
 * has occurred, such as in the native IPC layer.
 */
export class OperationError extends Error {
  /**
   * The code given to an `NOT_FOUND_ERR` `DOMException`
   */
  static get code () { return OPERATION_ERR }

  /**
   * `OperationError` class constructor.
   * @param {string} message
   * @param {number} [code]
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, OperationError)
    }
  }

  get name () {
    return 'OperationError'
  }

  get code () {
    return 'OPERATION_ERR'
  }
}

/**
 * An `TimeoutError` is an error type thrown when an operation timesout.
 */
export class TimeoutError extends Error {
  /**
   * The code given to an `TIMEOUT_ERR` `DOMException`
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/DOMException}
   */
  static get code () { return TIMEOUT_ERR }

  /**
   * `TimeoutError` class constructor.
   * @param {string} message
   */
  constructor (message, ...args) {
    super(message, ...args)

    if (typeof Error.captureStackTrace === 'function') {
      Error.captureStackTrace(this, TimeoutError)
    }
  }

  get name () {
    return 'TimeoutError'
  }

  get code () {
    return 'TIMEOUT_ERR'
  }
}
