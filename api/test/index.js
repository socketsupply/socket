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
import initContext from './context.js'
import {
  toElement,
  event as dispatchEventHelper,
  isElementVisible,
  waitFor,
  waitForText
} from './dom-helpers.js'

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

/**
 * @type {string}
 * @ignore
 */
let CACHED_FILE

/**
 * @returns {number} - The default timeout for tests in milliseconds.
 */
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
    /**
     * @type {string}
     * @ignore
     */
    this.name = name
    /**
     * @type {null|number}
     * @ignore
     */
    this._planned = null
    /**
     * @type {null|number}
     * @ignore
     */
    this._actual = null
    /**
     * @type {TestFn}
     * @ignore
     */
    this.fn = fn
    /**
     * @type {TestRunner}
     * @ignore
     */
    this.runner = runner
    /**
     * @type{{ pass: number, fail: number }}
     * @ignore
     */
    this._result = {
      pass: 0,
      fail: 0
    }
    /**
     * @type {boolean}
     * @ignore
     */
    this.done = false

    /**
     * @type {boolean}
     * @ignore
     */
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
   * Plan the number of assertions.
   *
   * @param {number} n
   * @returns {void}
   */
  plan (n) {
    this._planned = n
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

    /**
     * @type {Error | null}
     * @ignore
     */
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

    /**
     * @ignore
     */
    this._assert(
      pass, caught, expected, message || 'show throw', 'throws'
    )
  }

  // DOM Assertions

  /**
   * Sleep for ms with an optional msg
   *
   * @param {number} ms
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.sleep(100)
   * ```
   */
  async sleep (ms, msg) {
    msg = msg || `Sleep for ${ms}ms`
    await (new Promise((resolve) => setTimeout(resolve, ms)))
    this.pass(msg)
  }

  /**
   * Request animation frame with an optional msg. Falls back to a 0ms setTimeout when
   * tests are run headlessly.
   *
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.requestAnimationFrame()
   * ```
   */
  async requestAnimationFrame (msg) {
    if (document.hasFocus()) {
      // RAF only works when the window is focused
      await (new Promise(resolve => window.requestAnimationFrame(() => resolve())))
    } else {
      await (new Promise((resolve) => setTimeout(resolve, 0)))
    }
    if (msg) this.pass(msg)
  }

  /**
   * Dispatch the `click`` method on an element specified by selector.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.click('.class button', 'Click a button')
   * ```
   */
  async click (selector, msg) {
    msg = msg || `Clicked on ${typeof selector === 'string' ? selector : 'element'}`
    const el = toElement(selector)

    if (!(el instanceof window.HTMLElement)) throw new Error('selector needs to be instance of HTMLElement or resolve to one')
    el.click()
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   * Dispatch the click window.MouseEvent on an element specified by selector.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.eventClick('.class button', 'Click a button with an event')
   * ```
   */
  async eventClick (selector, msg) {
    const element = toElement(selector)
    msg = msg || `Fired click event on ${typeof selector === 'string' ? selector : 'element'}`
    dispatchEventHelper({
      event: new window.MouseEvent('click', {
        bubbles: true,
        cancelable: true,
        button: 0
      }),
      element
    })
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   *  Dispatch an event on the target.
   *
   * @param {string | Event} event - The event name or Event instance to dispatch.
   * @param {string|HTMLElement|Element} target - A CSS selector string, or an instance of HTMLElement, or Element to dispatch the event on.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.dispatchEvent('my-event', '#my-div', 'Fire the my-event event')
   * ```
   */
  async dispatchEvent (event, target, msg) {
    const element = toElement(target)
    msg = msg || `Fired event ${typeof event === 'string' ? ` ${event}` : ''} on ${typeof target === 'string' ? target : 'element'}`
    dispatchEventHelper({ event, element })
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   *  Call the focus method on element specified by selector.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.focus('#my-div')
   * ```
   */
  async focus (selector, msg) {
    msg = msg || `Focused on ${typeof selector === 'string' ? selector : 'element'}`
    const el = toElement(selector)
    if (!(el instanceof window.HTMLElement)) throw new Error('selector needs to be instance of HTMLElement or resolve to one')
    el.focus()
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   *  Call the blur method on element specified by selector.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.blur('#my-div')
   * ```
   */
  async blur (selector, msg) {
    msg = msg || `Blurred from ${typeof selector === 'string' ? selector : 'element'}`
    const el = toElement(selector)
    if (!(el instanceof window.HTMLElement)) throw new Error('selector needs to be instance of HTMLElement or resolve to one')
    el.blur()
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   * Consecutively set the str value of the element specified by selector to simulate typing.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {string} str - The string to type into the :focus element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.typeValue('#my-div', 'Hello World', 'Type "Hello World" into #my-div')
   * ```
   */
  async type (selector, str, msg) {
    msg = msg || `Typed by value ${str}${typeof selector === 'string' ? ` to ${selector}` : ''}`
    const el = toElement(selector)
    if (!('value' in el)) throw new Error('Element missing value attribute')
    for (const c of str.split('')) {
      await this.requestAnimationFrame()
      el.value = el.value != null ? el.value + c : c
      el.dispatchEvent(
        new Event('input', {
          bubbles: true,
          cancelable: true
        })
      )
    }
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   * appendChild an element el to a parent selector element.
   *
   * @param {string|HTMLElement|Element} parentSelector - A CSS selector string, or an instance of HTMLElement, or Element to appendChild on.
   * @param {HTMLElement|Element} el - A element to append to the parent element.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * const myElement = createElement('div')
   * await t.appendChild('#parent-selector', myElement, 'Append myElement into #parent-selector')
   * ```
   */
  async appendChild (parentSelector, el, msg = 'Appended child element') {
    const parentEl = toElement(parentSelector)
    const childEl = el
    parentEl.appendChild(childEl)
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   * Remove an element from the DOM.
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to remove from the DOM.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.removeElement('#dom-selector', 'Remove #dom-selector')
   * ```
   */
  async removeElement (selector, msg = 'Removed element') {
    const el = toElement(selector)
    el.remove()
    await this.requestAnimationFrame()
    this.pass(msg)
  }

  /**
   * Test if an element is visible
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to test visibility on.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.elementVisible('#dom-selector','Element is visible')
   * ```
   */
  async elementVisible (selector, msg) {
    msg = msg || `Element ${typeof selector === 'string' ? ` to ${selector}` : ''} is visible`
    const el = toElement(selector)
    const previousEl = el.previousElementSibling
    const visible = isElementVisible(el, previousEl)
    await this.requestAnimationFrame()
    this.ok(visible, msg)
  }

  /**
   * Test if an element is invisible
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element to test visibility on.
   * @param {string} [msg]
   * @returns {Promise<void>}
   *
   * @example
   * ```js
   * await t.elementInvisible('#dom-selector','Element is invisible')
   * ```
   */
  async elementInvisible (selector, msg) {
    msg = msg || `Element ${typeof selector === 'string' ? ` to ${selector}` : ''} is not visible`
    const el = toElement(selector)
    const previousEl = el.previousElementSibling
    const visible = isElementVisible(el, previousEl)
    await this.requestAnimationFrame()
    this.ok(!visible, msg)
  }

  /**
   * Test if an element is invisible
   *
   * @param {string|(() => HTMLElement|Element|null|undefined)} querySelectorOrFn - A query string or a function that returns an element.
   * @param {Object} [opts]
   * @param {boolean} [opts.visible] - The element needs to be visible.
   * @param {number} [opts.timeout] - The maximum amount of time to wait.
   * @param {string} [msg]
   * @returns {Promise<HTMLElement|Element|void>}
   *
   * @example
   * ```js
   * await t.waitFor('#dom-selector', { visible: true },'#dom-selector is on the page and visible')
   * ```
   */
  waitFor (querySelectorOrFn, opts, msg) {
    if (typeof opts === 'string') {
      msg = opts
      opts = {}
    }

    if (!opts) opts = {}

    let selector
    let selectorFn
    if (typeof querySelectorOrFn === 'string') {
      selector = querySelectorOrFn
    }
    if (typeof querySelectorOrFn === 'function') {
      selectorFn = querySelectorOrFn
    }

    if (!selector && !selectorFn) throw new Error('A query selector string or selector function is required')

    msg = msg || `Waiting for element ${typeof selector === 'string' ? ` ${selector}` : ''}`

    return waitFor({ ...opts, selector }, selectorFn)
      .then((el) => { this.pass(msg); return el })
      .catch(err => {
        this.ifError(err, msg)
      })
  }

  /**
   * @typedef {Object} WaitForTextOpts
   * @property {string} [text] - The text to wait for
   * @property {number} [timeout]
   * @property {Boolean} [multipleTags]
   * @property {RegExp} [regex] The regex to wait for
   */

  /**
   * Test if an element is invisible
   *
   * @param {string|HTMLElement|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
   * @param {WaitForTextOpts | string | RegExp} [opts]
   * @param {string} [msg]
   * @returns {Promise<HTMLElement|Element|void>}
   *
   * @example
   * ```js
   * await t.waitForText('#dom-selector', 'Text to wait for')
   * ```
   *
   * @example
   * ```js
   * await t.waitForText('#dom-selector', /hello/i)
   * ```
   *
   * @example
   * ```js
   * await t.waitForText('#dom-selector', {
   *   text: 'Text to wait for',
   *   multipleTags: true
   * })
   * ```
   */
  waitForText (selector, opts, msg) {
    const element = toElement(selector)

    if (typeof opts === 'string') {
      opts = {
        text: opts
      }
    }

    if (opts instanceof RegExp) {
      opts = {
        regex: opts
      }
    }

    if (!opts) throw new Error('Missing text, regex or search options object')

    msg = msg || `Waiting for text ${opts?.text ? ` ${opts.text}` : ''}${opts?.regex ? ` ${opts.regex}` : ''}`
    return waitForText({ ...opts, element })
      .then((el) => { this.pass(msg); return el })
      .catch(err => {
        this.ifError(err, msg)
      })
  }

  /**
   * Run a querySelector as an assert and also get the results
   *
   * @param {string} selector - A CSS selector string, or an instance of HTMLElement, or Element to select.
   * @param {string} [msg]
   * @returns {HTMLElement | Element}
   *
   * @example
   * ```js
   * const element = await t.querySelector('#dom-selector')
   * ```
   */
  querySelector (selector, msg) {
    const el = document.querySelector(selector)
    msg = msg || `querySelector(${selector})`
    this.ok(el, msg)
    return el
  }

  /**
   * Run a querySelectorAll as an assert and also get the results
   *
   * @param {string} selector - A CSS selector string, or an instance of HTMLElement, or Element to select.
   * @param {string} [msg]
   @returns {Array<HTMLElement | Element>}
   *
   * @example
   * ```js
   * const elements = await t.querySelectorAll('#dom-selector', '')
   * ```
   */
  querySelectorAll (selector, msg) {
    const elems = document.querySelectorAll(selector)
    const elementArray = Array.from(elems)
    msg = msg || `querySelectorAll(${selector})`
    this.ok(elementArray.length, msg)
    return elementArray
  }

  /**
   * Retrieves the computed styles for a given element.
   *
   * @param {string|Element} selector - The CSS selector or the Element object for which to get the computed styles.
   * @param {string} [msg] - An optional message to display when the operation is successful. Default message will be generated based on the type of selector.
   * @returns {CSSStyleDeclaration} - The computed styles of the element.
   * @throws {Error} - Throws an error if the element has no `ownerDocument` or if `ownerDocument.defaultView` is not available.
   *
   * @example
   * ```js
   * // Using CSS selector
   * const style = getComputedStyle('.my-element', 'Custom success message');
   * ```
   *
   * @example
   * ```js
   * // Using Element object
   * const el = document.querySelector('.my-element');
   * const style = getComputedStyle(el);
   * ```
   */
  getComputedStyle (selector, msg) {
    msg = msg || `Get computed style ${typeof selector === 'string' ? ` for ${selector}` : ''}`
    const el = toElement(selector)
    const ownerDocument = el.ownerDocument

    if (!ownerDocument || !ownerDocument.defaultView) {
      throw new Error('element has no ownerDocument')
    }

    const computedStyle = ownerDocument.defaultView.getComputedStyle(el)

    this.pass(msg)
    return computedStyle
  }

  /**
   * @param {boolean} pass
   * @param {unknown} actual
   * @param {unknown} expected
   * @param {string} description
   * @param {string} operator
   * @returns {void}
   * @ignore
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

    if (this._planned !== null) {
      this._actual = ((this._actual || 0) + 1)

      if (this._actual > this._planned) {
        throw new Error(`More tests than planned in TEST *${this.name}*`)
      }
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

    if (this._planned !== null) {
      if (this._planned > (this._actual || 0)) {
        throw new Error(`Test ended before the planned number
          planned: ${this._planned}
          actual: ${this._actual || 0}
          `
        )
      }
    }

    return this._result
  }
}

/**
 * @returns {string}
 * @ignore
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
 * @ignore
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
    /**
     * @type {(lines: string) => void}
     * @ignore
     */
    this.report = report || printLine

    /**
     * @type {Test[]}
     * @ignore
     */
    this.tests = []
    /**
     * @type {Test[]}
     * @ignore
     */
    this.onlyTests = []
    /**
     * @type {boolean}
     * @ignore
     */
    this.scheduled = false
    /**
     * @type {number}
     * @ignore
     */
    this._id = 0
    /**
     * @type {boolean}
     * @ignore
     */
    this.completed = false
    /**
     * @type {boolean}
     * @ignore
     */
    this.rethrowExceptions = true
    /**
     * @type {boolean}
     * @ignore
     */
    this.strict = false
    /**
    * @type {function | void}
    * @ignore
    */
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
 * @ignore
 */
function printLine (line) {
  console.log(line)
}

/**
 * @ignore
 */
export const GLOBAL_TEST_RUNNER = new TestRunner()

initContext(GLOBAL_TEST_RUNNER)

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
 * @typedef {{
 *    (name: string, fn?: TestFn): void
 *    only(name: string, fn?: TestFn): void
 *    skip(name: string, fn?: TestFn): void
 * }} testWithProperties
 * @ignore
 */

/**
 * @type {testWithProperties}
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
 * @ignore
 */
function rethrowImmediate (err) {
  setTimeout(rethrow, 0)

  /**
   * @returns {void}
   * @ignore
   */
  function rethrow () { throw err }
}

/**
 * JSON.stringify `thing` while preserving `undefined` values in
 * the output.
 *
 * @param {unknown} thing
 * @returns {string}
 * @ignore
 */
function toJSON (thing) {
  /**
   * @type {(_k: string, v: unknown) => unknown}
   * @ignore
   */
  const replacer = (_k, v) => (v === undefined) ? '_tz_undefined_tz_' : v

  const json = JSON.stringify(thing, replacer, '  ') || 'undefined'
  return json.replace(/"_tz_undefined_tz_"/g, 'undefined')
}
