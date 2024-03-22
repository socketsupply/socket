import fastDeepEqual from './test/fast-deep-equal.js'
import util from './util.js'

export class AssertionError extends Error {
  actual = null
  expected = null
  operator = null

  constructor (options) {
    super(options.message)
  }
}

export function assert (value, message = null) {
  if (value === undefined) {
    throw new AssertionError({
      operator: '==',
      message: 'No value argument passed to `assert()`',
      actual: value,
      expected: true
    })
  } else if (!value) {
    throw new AssertionError({
      operator: '==',
      message: message || 'The expression evaluated to a falsy value:',
      actual: value,
      expected: true
    })
  }
}

export function ok (value, message = null) {
  if (value === undefined) {
    throw new AssertionError({
      operator: '==',
      message: 'No value argument passed to `assert.ok()`',
      actual: value,
      expected: true
    })
  } else if (!value) {
    throw new AssertionError({
      operator: '==',
      message: message || 'The expression evaluated to a falsy value:',
      actual: value,
      expected: true
    })
  }
}

export function equal (actual, expected, message = null) {
  // eslint-disable-next-line
  if (actual != expected) {
    throw new AssertionError({
      operator: '==',
      message: message || `${util.inspect(actual)} == ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export function notEqual (actual, expected, message = null) {
  // eslint-disable-next-line
  if (actual == expected) {
    throw new AssertionError({
      operator: '!=',
      message: message || `${util.inspect(actual)} != ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export function strictEqual (actual, expected, message = null) {
  if (actual !== expected) {
    throw new AssertionError({
      operator: '===',
      message: message || `${util.inspect(actual)} === ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export function notStrictEqual (actual, expected, message = null) {
  if (actual === expected) {
    throw new AssertionError({
      operator: '!==',
      message: message || `${util.inspect(actual)} !== ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export function deepEqual (actual, expected, message = null) {
  if (fastDeepEqual(actual, expected)) {
    throw new AssertionError({
      operator: '==',
      message: message || `${util.inspect(actual)} == ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export function notDeepEqual (actual, expected, message = null) {
  if (!fastDeepEqual(actual, expected)) {
    throw new AssertionError({
      operator: '!=',
      message: message || `${util.inspect(actual)} != ${util.inspect(expected)}`,
      actual,
      expected
    })
  }
}

export default Object.assign(assert, {
  AssertionError,
  ok,
  equal,
  notEqual,
  strictEqual,
  notStrictEqual,
  deepEqual,
  notDeepEqual
})
