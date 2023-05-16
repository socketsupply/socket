// @ts-check
/**
 * @module Test
 *
 * Provides a test runner for Socket Runtime.
 *
 * Example usage:
 * ```js
 * import { test } from 'socket:test'
 *
 * test('test name', async t => {
 *   t.equal(1, 1)
 * })
 * ```
 */

import { format } from '../util.js'
import deepEqual from './fast-deep-equal.js'
import process from '../process.js'
import os from '../os.js'

const {
  SOCKET_TEST_RUNNER_TIMEOUT = getDefaultTestRunnerTimeout()
} = process.env

const NEW_LINE_REGEX = /\n/g
const OBJ_TO_STRING = Object.prototype.toString
const AT_REGEX = new RegExp(
  // non-capturing group for 'at '
  '^(?:[^\\s]*\\s*\\bat\\s+)' +
    // captures function call description
    '(?:(.*)\\s+\\()?' +
    // captures file path plus line no
    '((?:\\/|[a-zA-Z]:\\\\)[^:\\)]+:(\\d+)(?::(\\d+))?)\\)$'
)

/** @type {string} */
let CACHED_FILE

export function getDefaultTestRunnerTimeout () {
  if (os.platform() === 'win32') {
    return 2 * 1024
  } else {
    return 500
  }
}

/**
 * @typedef {(t: Test) => (void | Promise<void>)} TestFn
 */

/**
 * @class
 */
export class Test {
  /**
   * @constructor
   * @param {string} name
   * @param {TestFn} fn
   * @param {TestRunner} runner
   */
  constructor (name, fn, runner) {
    /** @type {string} */
    this.name = name
    /** @type {TestFn} */
    this.fn = fn
    /** @type {TestRunner} */
    this.runner = runner
    /** @type {{ pass: number, fail: number }} */
    this._result = {
      pass: 0,
      fail: 0
    }
    /** @type {boolean} */
    this.done = false

    /** @type {boolean} */
    this.strict = runner.strict
  }

  /**
   * @param {string} msg
   * @returns {void}
   */
  comment (msg) {
    this.runner.report('# ' + msg)
  }

  /**
   * @template T
   * @param {T} actual
   * @param {T} expected
   * @param {string} [msg]
   * @returns {void}
   */
  deepEqual (actual, expected, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      deepEqual(actual, expected), actual, expected,
      msg || 'should be equivalent', 'deepEqual'
    )
  }

  /**
   * @template T
   * @param {T} actual
   * @param {T} expected
   * @param {string} [msg]
   * @returns {void}
   */
  notDeepEqual (actual, expected, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      !deepEqual(actual, expected), actual, expected,
      msg || 'should not be equivalent', 'notDeepEqual'
    )
  }

  /**
   * @template T
   * @param {T} actual
   * @param {T} expected
   * @param {string} [msg]
   * @returns {void}
   */
  equal (actual, expected, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      // eslint-disable-next-line eqeqeq
      actual == expected, actual, expected,
      msg || 'should be equal', 'equal'
    )
  }

  /**
   * @param {unknown} actual
   * @param {unknown} expected
   * @param {string} [msg]
   * @returns {void}
   */
  notEqual (actual, expected, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      // eslint-disable-next-line eqeqeq
      actual != expected, actual, expected,
      msg || 'should not be equal', 'notEqual'
    )
  }

  /**
   * @param {string} [msg]
   * @returns {void}
   */
  fail (msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      false, 'fail called', 'fail not called',
      msg || 'fail called', 'fail'
    )
  }

  /**
   * @param {unknown} actual
   * @param {string} [msg]
   * @returns {void}
   */
  ok (actual, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      !!actual, actual, 'truthy value',
      msg || 'should be truthy', 'ok'
    )
  }

  /**
   * @param {string} [msg]
   * @returns {void}
   */
  pass (msg) {
    return this.ok(true, msg)
  }

  /**
   * @param {Error | null | undefined} err
   * @param {string} [msg]
   * @returns {void}
   */
  ifError (err, msg) {
    if (this.strict && !msg) throw new Error('tapzero msg required')
    this._assert(
      !err, err, 'no error', msg || String(err), 'ifError'
    )
  }

  /**
   * @param {Function} fn
   * @param {RegExp | any} [expected]
   * @param {string} [message]
   * @returns {void}
   */
  throws (fn, expected, message) {
    if (typeof expected === 'string') {
      message = expected
      expected = undefined
    }

    if (this.strict && !message) throw new Error('tapzero msg required')

    /** @type {Error | null} */
    let caught = null
    try {
      fn()
    } catch (err) {
      caught = /** @type {Error} */ (err)
    }

    let pass = !!caught

    if (expected instanceof RegExp) {
      pass = !!(caught && expected.test(caught.message))
    } else if (expected) {
      throw new Error(`t.throws() not implemented for expected: ${typeof expected}`)
    }

    this._assert(
      pass, caught, expected, message || 'show throw', 'throws'
    )
  }

  /**
   * @param {boolean} pass
   * @param {unknown} actual
   * @param {unknown} expected
   * @param {string} description
   * @param {string} operator
   * @returns {void}
   */
  _assert (
    pass, actual, expected,
    description, operator
  ) {
    if (this.done) {
      throw new Error(
        'assertion occurred after test was finished: ' + this.name
      )
    }

    const report = this.runner.report

    const prefix = pass ? 'ok' : 'not ok'
    const id = this.runner.nextId()
    report(`${prefix} ${id} ${description}`)

    if (pass) {
      this._result.pass++
      return
    }

    const atErr = new Error(description)
    let err = atErr
    if (actual && OBJ_TO_STRING.call(actual) === '[object Error]') {
      err = /** @type {Error} */ (actual)
      actual = err.message
    }

    this._result.fail++
    report('  ---')
    report(`    operator: ${operator}`)

    let ex = toJSON(expected) || 'undefined'
    let ac = toJSON(actual) || 'undefined'
    if (Math.max(ex.length, ac.length) > 65) {
      ex = ex.replace(NEW_LINE_REGEX, '\n      ')
      ac = ac.replace(NEW_LINE_REGEX, '\n      ')

      report(`    expected: |-\n      ${ex}`)
      report(`    actual:   |-\n      ${ac}`)
    } else {
      report(`    expected: ${ex}`)
      report(`    actual:   ${ac}`)
    }

    const at = findAtLineFromError(atErr)
    if (at) {
      report(`    at:       ${at}`)
    }

    report('    stack:    |-')
    const st = format(err || '').split('\n').slice(1)
    for (const line of st) {
      report(`      ${line}`)
    }

    report('  ...')
  }

  /**
   * @returns {Promise<{
   *   pass: number,
   *   fail: number
   * }>}
   */
  async run () {
    this.runner.report('# ' + this.name)
    const maybeP = this.fn(this)
    if (maybeP && typeof maybeP.then === 'function') {
      await maybeP
    }
    this.done = true
    return this._result
  }
}

/**
 * @returns {string}
 */
function getTapZeroFileName () {
  if (CACHED_FILE) return CACHED_FILE

  const e = new Error('temp')
  const lines = (e.stack || '').split('\n')

  for (const line of lines) {
    const m = AT_REGEX.exec(line)
    if (!m) {
      continue
    }

    let fileName = m[2]
    if (m[4] && fileName.endsWith(`:${m[4]}`)) {
      fileName = fileName.slice(0, fileName.length - m[4].length - 1)
    }
    if (m[3] && fileName.endsWith(`:${m[3]}`)) {
      fileName = fileName.slice(0, fileName.length - m[3].length - 1)
    }

    CACHED_FILE = fileName
    break
  }

  return CACHED_FILE || ''
}

/**
 * @param {Error} e
 * @returns {string}
 */
function findAtLineFromError (e) {
  const lines = (e.stack || '').split('\n')
  const dir = getTapZeroFileName()

  for (const line of lines) {
    const m = AT_REGEX.exec(line)
    if (!m) {
      continue
    }

    if (m[2].slice(0, dir.length) === dir) {
      continue
    }

    return `${m[1] || '<anonymous>'} (${m[2]})`
  }
  return ''
}

/**
 * @class
 */
export class TestRunner {
  /**
   * @constructor
   * @param {(lines: string) => void} [report]
   */
  constructor (report) {
    /** @type {(lines: string) => void} */
    this.report = report || printLine

    /** @type {Test[]} */
    this.tests = []
    /** @type {Test[]} */
    this.onlyTests = []
    /** @type {boolean} */
    this.scheduled = false
    /** @type {number} */
    this._id = 0
    /** @type {boolean} */
    this.completed = false
    /** @type {boolean} */
    this.rethrowExceptions = true
    /** @type {boolean} */
    this.strict = false
    /** @type {function | void} */
    this._onFinishCallback = undefined
  }

  /**
   * @returns {string}
   */
  nextId () {
    return String(++this._id)
  }

  /**
   * @param {string} name
   * @param {TestFn} fn
   * @param {boolean} only
   * @returns {void}
   */
  add (name, fn, only) {
    if (this.completed) {
      // TODO: calling add() after run()
      throw new Error('Cannot add() a test case after tests completed.')
    }
    const t = new Test(name, fn, this)
    const arr = only ? this.onlyTests : this.tests
    arr.push(t)
    if (!this.scheduled) {
      this.scheduled = true
      setTimeout(() => {
        const promise = this.run()
        if (this.rethrowExceptions) {
          promise.then(null, rethrowImmediate)
        }
      }, SOCKET_TEST_RUNNER_TIMEOUT)
    }
  }

  /**
   * @returns {Promise<void>}
   */
  async run () {
    const ts = this.onlyTests.length > 0
      ? this.onlyTests
      : this.tests

    this.report('TAP version 13')

    let total = 0
    let success = 0
    let fail = 0

    for (const test of ts) {
      // TODO: parallel execution
      const result = await test.run()

      total += result.fail + result.pass
      success += result.pass
      fail += result.fail
    }

    this.completed = true

    // timeout before reporting
    await new Promise((resolve) => setTimeout(resolve, 256))

    this.report('')
    this.report(`1..${total}`)
    this.report(`# tests ${total}`)
    this.report(`# pass  ${success}`)
    if (fail) {
      this.report(`# fail  ${fail}`)
    } else {
      this.report('')
      this.report('# ok')
    }

    if (this._onFinishCallback) {
      this._onFinishCallback({ total, success, fail })
    } else {
      if (fail) process.exit(1)
    }
  }

  /**
   * @param {(result: { total: number, success: number, fail: number }) => void} callback
   * @returns {void}
   */
  onFinish (callback) {
    if (typeof callback === 'function') {
      this._onFinishCallback = callback
    } else throw new Error('onFinish() expects a function')
  }
}

/**
 * @param {string} line
 * @returns {void}
 */
function printLine (line) {
  console.log(line)
}

export const GLOBAL_TEST_RUNNER = new TestRunner()

/**
 * @param {string} name
 * @param {TestFn} [fn]
 * @returns {void}
 */
export function only (name, fn) {
  if (!fn) return
  GLOBAL_TEST_RUNNER.add(name, fn, true)
}

/**
 * @param {string} _name
 * @param {TestFn} [_fn]
 * @returns {void}
 */
export function skip (_name, _fn) {}

/**
 * @param {boolean} strict
 * @returns {void}
 */
export function setStrict (strict) {
  GLOBAL_TEST_RUNNER.strict = strict
}

/**
 * @type {{
 *    (name: string, fn?: TestFn): void
 *    only(name: string, fn?: TestFn): void
 *    skip(name: string, fn?: TestFn): void
 * }}
 *
 * @param {string} name
 * @param {TestFn} [fn]
 * @returns {void}
 */
export function test (name, fn) {
  if (!fn) return
  GLOBAL_TEST_RUNNER.add(name, fn, false)
}
test.only = only
test.skip = skip

export default test

/**
 * @param {Error} err
 * @returns {void}
 */
function rethrowImmediate (err) {
  setTimeout(rethrow, 0)

  /**
   * @returns {void}
   */
  function rethrow () { throw err }
}

/**
 * JSON.stringify `thing` while preserving `undefined` values in
 * the output.
 *
 * @param {unknown} thing
 * @returns {string}
 */
function toJSON (thing) {
  /** @type {(_k: string, v: unknown) => unknown} */
  const replacer = (_k, v) => (v === undefined) ? '_tz_undefined_tz_' : v

  const json = JSON.stringify(thing, replacer, '  ') || 'undefined'
  return json.replace(/"_tz_undefined_tz_"/g, 'undefined')
}
