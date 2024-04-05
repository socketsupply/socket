import * as exports from './types.js'

const Uint8ArrayPrototype = Uint8Array.prototype
const TypedArrayPrototype = Object.getPrototypeOf(Uint8ArrayPrototype)
const TypedArray = TypedArrayPrototype.constructor

const AsyncGeneratorFunction = async function * () {}.constructor
const GeneratorFunction = function * () {}.constructor
const AsyncFunction = (async () => {}).constructor

const AsyncGeneratorPrototype = Object.getPrototypeOf(function * () {}()).constructor.prototype
const GeneratorPrototype = Object.getPrototypeOf(function * () {}()).constructor.prototype

const AsyncIteratorPrototype = Object.getPrototypeOf(AsyncGeneratorPrototype)
const IteratorPrototype = Object.getPrototypeOf(GeneratorPrototype)

const MapIteratorPrototype = Object.getPrototypeOf((new Map())[Symbol.iterator]())
const SetIteratorPrototype = Object.getPrototypeOf((new Set())[Symbol.iterator]())

/**
 * Returns `true` if input is an `Array`.
 * @param {any} input
 * @return {boolean}
 */
export const isArray = Array.isArray.bind(Array)

/**
 * Returns `true` if input is a plan `Object` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isPlainObject (input) {
  return (
    input !== null &&
    typeof input === 'object' &&
    Object.getPrototypeOf(input) === Object.prototype
  )
}

/**
 * Returns `true` if input is an `AsyncFunction`
 * @param {any} input
 * @return {boolean}
 */
export function isAsyncFunction (input) {
  return typeof input === 'function' && input instanceof AsyncFunction
}

/**
 * Returns `true` if input is an `Function`
 * @param {any} input
 * @return {boolean}
 */
export function isFunction (input) {
  return typeof input === 'function'
}

/**
 * Returns `true` if input is an `AsyncFunction` object.
 * @param {any} input
 * @return {boolean}
 */
export function isAsyncFunctionObject (input) {
  return typeof input === 'object' && input instanceof AsyncFunction
}

/**
 * Returns `true` if input is an `Function` object.
 * @param {any} input
 * @return {boolean}
 */
export function isFunctionObject (input) {
  return typeof input === 'object' && input instanceof Function
}

/**
 * Always returns `false`.
 * @param {any} input
 * @return {boolean}
 */
export function isExternal (input) {
  return false
}

/**
 * Returns `true` if input is a `Date` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isDate (input) {
  return input instanceof Date
}

/**
 * Returns `true` if input is an `arguments` object.
 * @param {any} input
 * @return {boolean}
 */
export function isArgumentsObject (input) {
  return (
    !Array.isArray(input) &&
    isPlainObject(input) &&
    Number.isFinite(input.length) &&
    typeof input[Symbol.iterator] === 'function' &&
    'callee' in input // access may throw error
  )
}

/**
 * Returns `true` if input is a `BigInt` object.
 * @param {any} input
 * @return {boolean}
 */
export function isBigIntObject (input) {
  return input instanceof BigInt
}

/**
 * Returns `true` if input is a `Boolean` object.
 * @param {any} input
 * @return {boolean}
 */
export function isBooleanObject (input) {
  return input instanceof Boolean
}

/**
 * Returns `true` if input is a `Number` object.
 * @param {any} input
 * @return {boolean}
 */
export function isNumberObject (input) {
  return input instanceof Number
}

/**
 * Returns `true` if input is a `String` object.
 * @param {any} input
 * @return {boolean}
 */
export function isStringObject (input) {
  return input instanceof String
}

/**
 * Returns `true` if input is a `Symbol` object.
 * @param {any} input
 * @return {boolean}
 */
export function isSymbolObject (input) {
  return input instanceof Symbol
}

/**
 * Returns `true` if input is native `Error` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isNativeError (input) {
  return input instanceof Error
}

/**
 * Returns `true` if input is a `RegExp` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isRegExp (input) {
  return input instanceof RegExp
}

/**
 * Returns `true` if input is a `GeneratorFunction`.
 * @param {any} input
 * @return {boolean}
 */
export function isGeneratorFunction (input) {
  return input instanceof GeneratorFunction
}

/**
 * Returns `true` if input is an `AsyncGeneratorFunction`.
 * @param {any} input
 * @return {boolean}
 */
export function isAsyncGeneratorFunction (input) {
  return input instanceof AsyncGeneratorFunction
}

/**
 * Returns `true` if input is an instance of a `Generator`.
 * @param {any} input
 * @return {boolean}
 */
export function isGeneratorObject (input) {
  // eslint-disable-next-line
  return input && typeof input === 'object' && GeneratorPrototype.isPrototypeOf(input)
}

/**
 * Returns `true` if input is a `Promise` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isPromise (input) {
  return input instanceof Promise
}

/**
 * Returns `true` if input is a `Map` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isMap (input) {
  return input instanceof Map
}

/**
 * Returns `true` if input is a `Set` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isSet (input) {
  return input instanceof Set
}

/**
 * Returns `true` if input is an instance of an `Iterator`.
 * @param {any} input
 * @return {boolean}
 */
export function isIterator (input) {
  // eslint-disable-next-line
  return input && typeof input === 'object' && IteratorPrototype.isPrototypeOf(input)
}

/**
 * Returns `true` if input is an instance of an `AsyncIterator`.
 * @param {any} input
 * @return {boolean}
 */
export function isAsyncIterator (input) {
  // eslint-disable-next-line
  return input && typeof input === 'object' && AsyncIteratorPrototype.isPrototypeOf(input)
}

/**
 * Returns `true` if input is an instance of a `MapIterator`.
 * @param {any} input
 * @return {boolean}
 */
export function isMapIterator (input) {
  // eslint-disable-next-line
  return input && typeof input === 'object' && MapIteratorPrototype.isPrototypeOf(input)
}

/**
 * Returns `true` if input is an instance of a `SetIterator`.
 * @param {any} input
 * @return {boolean}
 */
export function isSetIterator (input) {
  // eslint-disable-next-line
  return input && typeof input === 'object' && SetIteratorPrototype.isPrototypeOf(input)
}

/**
 * Returns `true` if input is a `WeakMap` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isWeakMap (input) {
  return input instanceof WeakMap
}

/**
 * Returns `true` if input is a `WeakSet` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isWeakSet (input) {
  return input instanceof WeakSet
}

/**
 * Returns `true` if input is an `ArrayBuffer` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isArrayBuffer (input) {
  return input instanceof ArrayBuffer
}

/**
 * Returns `true` if input is an `DataView` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isDataView (input) {
  return input instanceof DataView
}

/**
 * Returns `true` if input is a `SharedArrayBuffer`.
 * This will always return `false` if a `SharedArrayBuffer`
 * type is not available.
 * @param {any} input
 * @return {boolean}
 */
export function isSharedArrayBuffer (input) {
  if (typeof globalThis.SharedArrayBuffer === 'function') {
    return input instanceof globalThis.SharedArrayBuffer
  }
  return false
}

/**
 * Not supported. This function will return `false` always.
 * @param {any} input
 * @return {boolean}
 */
export function isProxy (input) {
  return false
}

/**
 * Returns `true` if input looks like a module namespace object.
 * @param {any} input
 * @return {boolean}
 */
export function isModuleNamespaceObject (input) {
  return (
    input &&
    typeof input === 'object' &&
    Object.getPrototypeOf(input) === null &&
    Reflect.isExtensible(input) === false &&
    input[Symbol.toStringTag]?.() === 'Module'
  )
}

/**
 * Returns `true` if input is an `ArrayBuffer` of `SharedArrayBuffer`.
 * @param {any} input
 * @return {boolean}
 */
export function isAnyArrayBuffer (input) {
  return isArrayBuffer(input) || isSharedArrayBuffer(input)
}

/**
 * Returns `true` if input is a "boxed" primitive.
 * @param {any} input
 * @return {boolean}
 */
export function isBoxedPrimitive (input) {
  return (
    isSymbolObject(input) ||
    isBigIntObject(input) ||
    isNumberObject(input) ||
    isStringObject(input) ||
    isBooleanObject(input) ||
    isFunctionObject(input) ||
    isAsyncFunctionObject(input)
  )
}

/**
 * Returns `true` if input is an `ArrayBuffer` view.
 * @param {any} input
 * @return {boolean}
 */
export function isArrayBufferView (input) {
  return isDataView(input) || ArrayBuffer.isView(input)
}

/**
 * Returns `true` if input is a `TypedArray` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isTypedArray (input) {
  return input instanceof TypedArray
}

/**
 * Returns `true` if input is an `Uint8Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isUint8Array (input) {
  return input instanceof Uint8Array
}

/**
 * Returns `true` if input is an `Uint8ClampedArray` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isUint8ClampedArray (input) {
  return input instanceof Uint8ClampedArray
}

/**
 * Returns `true` if input is an `Uint16Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isUint16Array (input) {
  return input instanceof Uint16Array
}

/**
 * Returns `true` if input is an `Uint32Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isUint32Array (input) {
  return input instanceof Uint32Array
}

/**
 * Returns `true` if input is an Int8Array`` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isInt8Array (input) {
  return input instanceof Int8Array
}

/**
 * Returns `true` if input is an `Int16Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isInt16Array (input) {
  return input instanceof Int16Array
}

/**
 * Returns `true` if input is an `Int32Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isInt32Array (input) {
  return input instanceof Int32Array
}

/**
 * Returns `true` if input is an `Float32Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isFloat32Array (input) {
  return input instanceof Float32Array
}

/**
 * Returns `true` if input is an `Float64Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isFloat64Array (input) {
  return input instanceof Float64Array
}

/**
 * Returns `true` if input is an `BigInt64Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isBigInt64Array (input) {
  return input instanceof BigInt64Array
}

/**
 * Returns `true` if input is an `BigUint64Array` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isBigUint64Array (input) {
  return input instanceof BigUint64Array
}

/**
 * @ignore
 * @param {any} input
 * @return {boolean}
 */
export function isKeyObject (input) {
  return false
}

/**
 * Returns `true` if input is a `CryptoKey` instance.
 * @param {any} input
 * @return {boolean}
 */
export function isCryptoKey (input) {}

export default exports
