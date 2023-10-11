/* global MutationObserver */
import { fetch, Headers, Request, Response } from '../fetch.js'
import { URL, URLPattern, URLSearchParams } from '../url.js'
import Notification from '../notification.js'
import geolocation from './geolocation.js'
import permissions from './permissions.js'

import ipc from '../ipc.js'

let applied = false

export function init () {
  if (applied || !globalThis.window) return

  function install (implementations, target = globalThis, prefix) {
    for (let name in implementations) {
      const implementation = implementations[name]

      if (typeof prefix === 'string') {
        name = `${prefix}.${name}`
      }

      const actualName = name.split('.').slice(-1)[0]

      if (typeof target[actualName] === 'object' && target[actualName] !== null) {
        for (const key in implementation) {
          const nativeImplementation = target[actualName][key] || null
          // let this fail, the environment implementation may not be writable
          try {
            target[actualName][key] = implementation[key]
          } catch {}

          if (nativeImplementation !== null) {
            const nativeName = ['_', 'native', ...name.split('.'), key].join('_')
            Object.defineProperty(globalThis, nativeName, {
              enumerable: false,
              configurable: false,
              value: nativeImplementation
            })
          }
        }
      } else {
        const nativeImplementation = target[actualName] || null
        // let this fail, the environment implementation may not be writable
        try {
          target[actualName] = implementation
        } catch {}

        if (nativeImplementation !== null) {
          const nativeName = ['_', 'native', ...name.split('.')].join('_')
          Object.defineProperty(globalThis, nativeName, {
            enumerable: false,
            configurable: false,
            value: nativeImplementation
          })
        }
      }
    }
  }

  if (
    typeof globalThis.webkitSpeechRecognition === 'function' &&
    typeof globalThis.SpeechRecognition !== 'function'
  ) {
    globalThis.SpeechRecognition = globalThis.webkitSpeechRecognition
  }

  // globals
  install({
    // url
    URL,
    URLPattern,
    URLSearchParams,

    // fetch
    fetch,
    Headers,
    Request,
    Response,

    // notifications
    Notification
  })

  // navigator
  install({ geolocation, permissions }, globalThis.navigator, 'navigator')

  applied = true
  // create <title> tag in document if it doesn't exist
  globalThis.document.title ||= ''
  // initial value
  globalThis.addEventListener('DOMContentLoaded', () => {
    const title = globalThis.document.title
    if (title.length !== 0) {
      const index = globalThis.__args.index
      const o = new URLSearchParams({ value: title, index }).toString()
      ipc.postMessage(`ipc://window.setTitle?${o}`)
    }
  })

  //
  // globalThis.document is uncofigurable property so we need to use MutationObserver here
  //
  const observer = new MutationObserver((mutationList) => {
    for (const mutation of mutationList) {
      if (mutation.type === 'childList') {
        const index = globalThis.__args.index
        const title = mutation.addedNodes[0].textContent
        const o = new URLSearchParams({ value: title, index }).toString()
        ipc.postMessage(`ipc://window.setTitle?${o}`)
      }
    }
  })

  const titleElement = document.querySelector('head > title')
  if (titleElement) {
    observer.observe(titleElement, { childList: true })
  }
}

export default init()
