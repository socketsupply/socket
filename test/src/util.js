import util, { hasOwnProperty } from '../../util.js'
import Buffer from '../../buffer.js'
import { test } from '@socketsupply/tapzero'

test('util.hasOwnProperty', (t) => {
  const obj = { foo: 'bar' }
  t.ok(hasOwnProperty(obj, 'foo'), 'util.hasOwnProperty returns true for own properties')
  t.ok(!hasOwnProperty(obj, 'bar'), 'util.hasOwnProperty returns false for non-own properties')
})

test('util.isTypedArray', (t) => {
  const arr = new Uint8Array(8)
  t.ok(util.isTypedArray(arr), 'util.isTypedArray returns true for typed arrays')
  t.ok(!util.isTypedArray([]), 'util.isTypedArray returns false for non-typed arrays')
})

test('util.isArrayLike', (t) => {
  const arr = new Uint8Array(8)
  t.ok(util.isArrayLike(arr), 'util.isArrayLike returns true for typed arrays')
  t.ok(util.isArrayLike([]), 'util.isArrayLike returns true for arrays')
  t.ok(!util.isArrayLike({}), 'util.isArrayLike returns false for objects')
})

test('util.isArrayBufferView', (t) => {
  const arr = new Uint8Array(8)
  t.ok(util.isArrayBufferView(arr), 'util.isArrayBufferView returns true for typed arrays')
  t.ok(!util.isArrayBufferView([]), 'util.isArrayBufferView returns false for arrays')
  t.ok(!util.isArrayBufferView({}), 'util.isArrayBufferView returns false for objects')
})

test('util.isAsyncFunction', (t) => {
  const fn = async () => {}
  t.ok(util.isAsyncFunction(fn), 'util.isAsyncFunction returns true for async functions')
  t.ok(!util.isAsyncFunction(() => {}), 'util.isAsyncFunction returns false for non-async functions')
})

test('util.isArgumentsObject', (t) => {
  const args = (function () { return arguments })()
  t.ok(util.isArgumentsObject(args), 'util.isArgumentsObject returns true for arguments objects')
  // should it?
  // t.ok(!util.isArgumentsObject({}), 'util.isArgumentsObject returns false for non-arguments objects')
})

test('util.isEmptyObject', (t) => {
  const obj = {}
  t.ok(util.isEmptyObject(obj), 'util.isEmptyObject returns true for empty objects')
  t.ok(!util.isEmptyObject({ foo: 'bar' }), 'util.isEmptyObject returns false for non-empty objects')
})

test('util.isObject', (t) => {
  const obj = {}
  t.ok(util.isObject(obj), 'util.isObject returns true for objects')
  // Should it?
  // t.ok(!util.isObject([]), 'util.isObject returns false for arrays')
  t.ok(!util.isObject('foo'), 'util.isObject returns false for strings')
  t.ok(!util.isObject(1), 'util.isObject returns false for numbers')
  t.ok(!util.isObject(true), 'util.isObject returns false for booleans')
  t.ok(!util.isObject(null), 'util.isObject returns false for null')
  t.ok(!util.isObject(undefined), 'util.isObject returns false for undefined')
})

test('util.isPlainObject', (t) => {
  const obj = {}
  t.ok(util.isPlainObject(obj), 'util.isPlainObject returns true for plain objects')
  t.ok(!util.isPlainObject([]), 'util.isPlainObject returns false for arrays')
  t.ok(!util.isPlainObject('foo'), 'util.isPlainObject returns false for strings')
  t.ok(!util.isPlainObject(1), 'util.isPlainObject returns false for numbers')
  t.ok(!util.isPlainObject(true), 'util.isPlainObject returns false for booleans')
  t.ok(!util.isPlainObject(null), 'util.isPlainObject returns false for null')
  t.ok(!util.isPlainObject(undefined), 'util.isPlainObject returns false for undefined')
  t.ok(!util.isPlainObject(new Date()), 'util.isPlainObject returns false for Date objects')
})

test('util.isBufferLike', (t) => {
  const buf = Buffer.from('foo')
  t.ok(util.isBufferLike(buf), 'util.isBufferLike returns true for Buffer objects')
  t.ok(util.isBufferLike(new Uint8Array(8)), 'util.isBufferLike returns true for Uint8Array objects')
  t.ok(!util.isBufferLike('foo'), 'util.isBufferLike returns false for strings')
  t.ok(!util.isBufferLike(1), 'util.isBufferLike returns false for numbers')
  t.ok(!util.isBufferLike(true), 'util.isBufferLike returns false for booleans')
  t.ok(!util.isBufferLike(null), 'util.isBufferLike returns false for null')
  t.ok(!util.isBufferLike(undefined), 'util.isBufferLike returns false for undefined')
})

test('util.isFunction', (t) => {
  const fn = () => {}
  t.ok(util.isFunction(fn), 'util.isFunction returns true for functions')
  t.ok(!util.isFunction('foo'), 'util.isFunction returns false for strings')
  t.ok(!util.isFunction(1), 'util.isFunction returns false for numbers')
  t.ok(!util.isFunction(true), 'util.isFunction returns false for booleans')
  t.ok(!util.isFunction(null), 'util.isFunction returns false for null')
  t.ok(!util.isFunction(undefined), 'util.isFunction returns false for undefined')
  t.ok(!util.isFunction({}), 'util.isFunction returns false for objects')
  t.ok(!util.isFunction([]), 'util.isFunction returns false for arrays')

  const asyncFn = async () => {}
  t.ok(util.isFunction(asyncFn), 'util.isFunction returns true for async functions')
})

test('util.isErrorLike', (t) => {
  const err = new Error('foo')
  t.ok(util.isErrorLike(err), 'util.isErrorLike returns true for Error objects')
  t.ok(!util.isErrorLike('foo'), 'util.isErrorLike returns false for strings')
  t.ok(!util.isErrorLike(1), 'util.isErrorLike returns false for numbers')
  t.ok(!util.isErrorLike(true), 'util.isErrorLike returns false for booleans')
  t.ok(!util.isErrorLike(null), 'util.isErrorLike returns false for null')
  t.ok(!util.isErrorLike(undefined), 'util.isErrorLike returns false for undefined')
  t.ok(!util.isErrorLike({}), 'util.isErrorLike returns false for objects')
  t.ok(!util.isErrorLike([]), 'util.isErrorLike returns false for arrays')
  const errLike = { message: 'foo', name: 'Error' }
  t.ok(util.isErrorLike(errLike), 'util.isErrorLike returns true for objects with message and name properties')
})

test('util.isPromiseLike', (t) => {
  const promise = Promise.resolve()
  t.ok(util.isPromiseLike(promise), 'util.isPromiseLike returns true for Promise objects')
  t.ok(!util.isPromiseLike('foo'), 'util.isPromiseLike returns false for strings')
  t.ok(!util.isPromiseLike(1), 'util.isPromiseLike returns false for numbers')
  t.ok(!util.isPromiseLike(true), 'util.isPromiseLike returns false for booleans')
  t.ok(!util.isPromiseLike(null), 'util.isPromiseLike returns false for null')
  t.ok(!util.isPromiseLike(undefined), 'util.isPromiseLike returns false for undefined')
  t.ok(!util.isPromiseLike({}), 'util.isPromiseLike returns false for objects')
  t.ok(!util.isPromiseLike([]), 'util.isPromiseLike returns false for arrays')
  const promiseLike = { then: () => {} }
  t.ok(util.isPromiseLike(promiseLike), 'util.isPromiseLike returns true for objects with a then method')
  const notPromiseLike = { then: 'foo' }
  t.ok(!util.isPromiseLike(notPromiseLike), 'util.isPromiseLike returns false for objects with a then property that is not a function')
})

test('util.toProperCase', (t) => {
  t.equal(util.toProperCase('foo'), 'Foo', 'util.toProperCase returns a string with the first letter capitalized')
  t.equal(util.toProperCase('foo bar'), 'Foo bar', 'util.toProperCase returns a string with the first letter capitalized')
  t.equal(util.toProperCase('foo bar-baz'), 'Foo bar-baz', 'util.toProperCase returns a string with the first letter capitalized')
})

test('util.rand64', (t) => {
  const randoms = Array.from({ length: 10 }, _ => util.rand64())
  t.ok(randoms.every(b => typeof b === 'bigint'), 'util.rand64 returns a bigint')
  t.ok(randoms.some(b => b !== randoms[9]), 'util.rand64 returns a different bigint each time')
})

test('util.splitBuffer', (t) => {
  const buf = Buffer.from('foobar')
  const [a, b, c] = util.splitBuffer(buf, 2)
  t.equal(a.toString(), 'fo', 'util.splitBuffer returns an array of buffers')
  t.equal(b.toString(), 'ob', 'util.splitBuffer returns an array of buffers')
  t.equal(c.toString(), 'ar', 'util.splitBuffer returns an array of buffers')
})

test('util.InvertedPromise', (t) => {})

test('util.clamp', (t) => {
  t.equal(util.clamp(0, 0, 1), 0, 'util.clamp returns the lower bound if the value is less than the lower bound')
  t.equal(util.clamp(1, 0, 1), 1, 'util.clamp returns the upper bound if the value is greater than the upper bound')
  t.equal(util.clamp(0.5, 0, 1), 0.5, 'util.clamp returns the value if it is between the lower and upper bounds')
})

test('util.promisify', async (t) => {
  const fn = (a, b, cb) => cb(null, a + b)
  const promisified = util.promisify(fn)
  t.ok(util.isFunction(promisified), 'util.promisify returns a function')
  const res = await promisified(1, 2)
  t.equal(res, 3, 'util.promisify returns a function that returns a promise')
})
