// @ts-check
import * as exports from './harness.js'
export default exports

/**
 * @typedef {import('./index').Test} Test
 * @typedef {(t: Test) => Promise<void> | void} TestCase
 * @typedef {{
 *    bootstrap(): Promise<void>
 *    close(): Promise<void>
 * }} Harness
 */

/**
 * @template {Harness} T
 * @typedef {{
 *    (
 *      name: string,
 *      cb?: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 *    (
 *      name: string,
 *      opts: object,
 *      cb: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 *    only(
 *      name: string,
 *      cb?: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 *    only(
 *      name: string,
 *      opts: object,
 *      cb: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 *    skip(
 *      name: string,
 *      cb?: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 *    skip(
 *      name: string,
 *      opts: object,
 *      cb: (harness: T, test: Test) => (void | Promise<void>)
 *    ): void;
 * }} TapeTestFn
 */

/**
 * @template {Harness} T
 * @param {import('./index.js')} tapzero
 * @param {new (options: object) => T} harnessClass
 * @returns {TapeTestFn<T>}
 */
export function wrapHarness (tapzero, harnessClass) {
  const harness = new TapeHarness(tapzero, harnessClass)

  test.only = harness.only.bind(harness)
  test.skip = harness.skip.bind(harness)

  return test

  /**
   * @param {string} testName
   * @param {object} [options]
   * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
   * @returns {void}
   */
  function test (testName, options, fn) {
    return harness.test(testName, options, fn)
  }
}

/**
 * @template {Harness} T
 */
export class TapeHarness {
  /**
   * @param {import('./index.js')} tapzero
   * @param {new (options: object) => T} harnessClass
   */
  constructor (tapzero, harnessClass) {
    /** @type {import('./index.js')} */
    this.tapzero = tapzero
    /** @type {new (options: object) => T} */
    this.harnessClass = harnessClass
  }

  /**
   * @param {string} testName
   * @param {object} [options]
   * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
   * @returns {void}
   */
  test (testName, options, fn) {
    this._test(this.tapzero.test, testName, options, fn)
  }

  /**
   * @param {string} testName
   * @param {object} [options]
   * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
   * @returns {void}
   */
  only (testName, options, fn) {
    this._test(this.tapzero.only, testName, options, fn)
  }

  /**
   * @param {string} testName
   * @param {object} [options]
   * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
   * @returns {void}
   */
  skip (testName, options, fn) {
    this._test(this.tapzero.skip, testName, options, fn)
  }

  /**
   * @param {(str: string, fn?: TestCase) => void} tapzeroFn
   * @param {string} testName
   * @param {object} [options]
   * @param {(harness: T, test: Test) => (void | Promise<void>)} [fn]
   * @returns {void}
   */
  _test (tapzeroFn, testName, options, fn) {
    if (!fn && typeof options === 'function') {
      fn = /** @type {(h: T, test: Test) => void} */ (options)
      options = {}
    }

    if (!fn) {
      return tapzeroFn(testName)
    }
    const testFn = fn

    tapzeroFn(testName, async (assert) => {
      return this._onAssert(assert, options || {}, testFn)
    })
  }

  /**
   * @param {Test} assert
   * @param {object} options
   * @param {(harness: T, test: Test) => (void | Promise<void>)} fn
   * @returns {Promise<void>}
   */
  async _onAssert (assert, options, fn) {
    Reflect.set(options, 'assert', assert)

    const Harness = this.harnessClass
    const harness = new Harness(options)

    await harness.bootstrap()

    try {
      await fn(harness, assert)
    } finally {
      await harness.close()
    }
  }
}
