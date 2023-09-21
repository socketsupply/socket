// @ts-check
/**
 * @module Test.DOM-helpers
 *
 * Provides a test runner for Socket Runtime.
 *
 *
 * Example usage:
 * ```js
 * import { test } from 'socket:test/dom-helpers.js'
 * ```
 *
 */
// Based on @socketsupply/test-dom and vhs-tape.

const SECOND = 1000
const defaultTimeout = 5 * SECOND

/**
 * Converts querySelector string to an HTMLElement or validates an existing HTMLElement.
 *
 * @export
 * @param {string|Element} selector - A CSS selector string, or an instance of HTMLElement, or Element.
 * @returns {Element} The HTMLElement, Element, or Window that corresponds to the selector.
 * @throws {Error} Throws an error if the `selector` is not a string that resolves to an HTMLElement or not an instance of HTMLElement, Element, or Window.
 *
 */
export function toElement (selector) {
  if (typeof selector === 'string') selector = document.querySelector(selector)
  if (!(
    selector instanceof window.HTMLElement ||
    selector instanceof window.Element
  )) throw new Error('stringOrElement needs to be an instance of HTMLElement or a querySelector that resolves to a HTMLElement')
  return selector
}

/**
 * Waits for an element to appear in the DOM and resolves the promise when it does.
 *
 * @export
 * @param {Object} args - Configuration arguments.
 * @param {string} [args.selector] - The CSS selector to look for.
 * @param {boolean} [args.visible=true] - Whether the element should be visible.
 * @param {number} [args.timeout=defaultTimeout] - Time in milliseconds to wait before rejecting the promise.
 * @param {() => HTMLElement | Element | null | undefined} [lambda] - An optional function that returns the element. Used if the `selector` is not provided.
 * @returns {Promise<Element|HTMLElement|void>} - A promise that resolves to the found element.
 *
 * @throws {Error} - Throws an error if neither `lambda` nor `selector` is provided.
 * @throws {Error} - Throws an error if the element is not found within the timeout.
 *
 * @example
 * ```js
 * waitFor({ selector: '#my-element', visible: true, timeout: 5000 })
 *   .then(el => console.log('Element found:', el))
 *   .catch(err => console.log('Element not found:', err));
 * ```
 */
export function waitFor (args, lambda) {
  return new Promise((resolve, reject) => {
    const {
      selector,
      visible = true,
      timeout = defaultTimeout
    } = args

    if (!lambda && selector) {
      lambda = () => document.querySelector(selector)
    }

    const interval = setInterval(() => {
      if (!lambda) {
        throw new Error('lambda or selector required')
      }

      const el = lambda()
      if (el) {
        if (visible && !isElementVisible(el)) return
        clearTimeout(timer)
        return resolve(el)
      }
    }, 50)

    const timer = setTimeout(() => {
      clearInterval(interval)
      const wantsVisable = visible ? 'A visible selector' : 'A Selector'
      reject(new Error(`${wantsVisable} was not found after ${timeout}ms (${selector})`))
    }, timeout)
  })
}

/**
 * Waits for an element's text content to match a given string or regular expression.
 *
 * @export
 * @param {Object} args - Configuration arguments.
 * @param {Element} args.element - The root element from which to begin searching.
 * @param {string} [args.text] - The text to search for within elements.
 * @param {RegExp} [args.regex] - A regular expression to match against element text content.
 * @param {boolean} [args.multipleTags=false] - Whether to look for text across multiple sibling elements.
 * @param {number} [args.timeout=defaultTimeout] - Time in milliseconds to wait before rejecting the promise.
 * @returns {Promise<Element|HTMLElement|void>} - A promise that resolves to the found element or null.
 *
 * @example
 * ```js
 * waitForText({ element: document.body, text: 'Hello', timeout: 5000 })
 *   .then(el => console.log('Element found:', el))
 *   .catch(err => console.log('Element not found:', err));
 * ```
 */
export function waitForText (args) {
  return waitFor({
    timeout: args.timeout
  }, () => {
    const {
      element,
      text,
      regex,
      multipleTags
    } = args

    const elems = []

    let maxLoop = 10000
    const stack = [element]
    // Walk the DOM tree breadth first and build up a list of
    // elements with the leafs last.
    while (stack.length > 0 && maxLoop-- >= 0) {
      const current = stack.pop()
      if (current && current.children.length > 0) {
        stack.push(...current.children)
        elems.push(...current.children)
      }
    }

    // Loop over children in reverse to scan the LEAF nodes first.
    let match = null
    for (let i = elems.length - 1; i >= 0; i--) {
      const node = elems[i]
      if (!node.textContent) continue

      if (regex && regex.test(node.textContent)) {
        return node
      }

      if (text && node.textContent?.includes(text)) {
        return node
      }

      if (text && multipleTags) {
        if (text[0] !== (node.textContent)[0]) continue

        // if equal, check the sibling nodes
        let sibling = node.nextSibling
        let i = 1

        // while there is a potential match, keep checking the siblings
        while (i < text.length) {
          if (sibling && (sibling.textContent === text[i])) {
            // is equal still, check the next sibling
            sibling = sibling.nextSibling
            i++
            match = node.parentElement
          } else {
            if (i === (text.length - 1)) return node.parentElement
            match = null
            break
          }
        }
      }
    }

    return match
  })
}

/**
 * @export
 * @param {Object} args - Arguments
 * @param {string | Event} args.event - The event to dispatch.
 * @param {HTMLElement | Element | window} [args.element=window] - The element to dispatch the event on.
 * @returns {void}
 *
 * @throws {Error} Throws an error if the `event` is not a string that can be converted to a CustomEvent or not an instance of Event.
 */
export function event (args) {
  let {
    event,
    element = window
  } = args

  if (typeof event === 'string') {
    event = new window.CustomEvent(event)
  }

  if (typeof event !== 'object') {
    throw new Error('event should be of type Event')
  }

  element.dispatchEvent(event)
}

/**
 * @export
 * Copy pasted from https://raw.githubusercontent.com/testing-library/jest-dom/master/src/to-be-visible.js
 * @param {Element | HTMLElement} element
 * @param {Element | HTMLElement} [previousElement]
 * @returns {boolean}
 */
export function isElementVisible (element, previousElement) {
  return (
    isStyleVisible(element) &&
        isAttributeVisible(element, previousElement) &&
        (!element.parentElement || isElementVisible(element.parentElement, element))
  )
}

/**
 * @param {Element | HTMLElement} element
 * @returns {boolean}
 * @ignore
 */
function isStyleVisible (element) {
  const ownerDocument = element.ownerDocument
  if (!ownerDocument || !ownerDocument.defaultView) {
    throw new Error('element has no ownerDocument')
  }

  const {
    display,
    visibility,
    opacity
  } = ownerDocument.defaultView.getComputedStyle(element)

  return (
    display !== 'none' &&
        visibility !== 'hidden' &&
        visibility !== 'collapse' &&
        opacity !== '0' &&
        Number(opacity) !== 0
  )
}

/**
 * @param {Element | HTMLElement} element
 * @param {Element | HTMLElement} [previousElement]
 * @returns {boolean}
 * @ignore
 */
function isAttributeVisible (element, previousElement) {
  return (
    !element.hasAttribute('hidden') &&
        (element.nodeName === 'DETAILS' && previousElement?.nodeName !== 'SUMMARY'
          ? element.hasAttribute('open')
          : true)
  )
}
