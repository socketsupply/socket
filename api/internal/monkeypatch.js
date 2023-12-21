/* global MutationObserver */
import { fetch, Headers, Request, Response } from '../fetch.js'
import { URL, URLPattern, URLSearchParams } from '../url.js'
import { ApplicationURLEvent } from './events.js'
import Notification from '../notification.js'
import geolocation from './geolocation.js'
import permissions from './permissions.js'
import WebAssembly from './webassembly.js'

import {
  File,
  FileSystemHandle,
  FileSystemFileHandle,
  FileSystemDirectoryHandle,
  FileSystemWritableFileStream
} from '../fs/web.js'

import {
  showDirectoryPicker,
  showOpenFilePicker,
  showSaveFilePicker
} from './pickers.js'

import ipc from '../ipc.js'

let applied = false
const natives = {}
const patches = {}

export function init () {
  if (applied || !globalThis.window) {
    return { natives, patches }
  }

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
          if (nativeImplementation === implementation[key]) {
            continue
          }
          // let this fail, the environment implementation may not be writable
          try {
            target[actualName][key] = implementation[key]
          } catch {}

          patches[actualName] ??= {}
          patches[actualName][key] = implementation[key]
          if (nativeImplementation !== null) {
            const nativeName = ['_', 'native', ...name.split('.'), key].join('_')
            natives[name] = nativeImplementation
            Object.defineProperty(globalThis, nativeName, {
              enumerable: false,
              configurable: false,
              value: nativeImplementation
            })
          }
        }
      } else {
        const nativeImplementation = target[actualName] || null
        if (nativeImplementation !== implementation) {
          // let this fail, the environment implementation may not be writable
          try {
            target[actualName] = implementation
          } catch {}

          patches[actualName] = implementation
          if (
            (typeof nativeImplementation === 'function' && nativeImplementation.prototype) &&
            (typeof implementation === 'function' && implementation.prototype)
          ) {
            const nativeDescriptors = Object.getOwnPropertyDescriptors(nativeImplementation.prototype)
            const descriptors = Object.getOwnPropertyDescriptors(implementation.prototype)
            implementation[Symbol.species] = nativeImplementation
            for (const key in nativeDescriptors) {
              const nativeDescriptor = nativeDescriptors[key]
              const descriptor = descriptors[key]

              if (key === 'constructor') {
                continue
              }

              if (descriptor) {
                if (nativeDescriptor.set && nativeDescriptor.get) {
                  descriptors[key] = { ...nativeDescriptor, ...descriptor }
                } else {
                  descriptors[key] = { writable: true, configurable: true, ...descriptor }
                }
              } else {
                descriptors[key] = { writable: true, configurable: true }
              }

              if (descriptors[key] && typeof descriptors[key] === 'object') {
                if ('get' in descriptors[key] && typeof descriptors[key].get !== 'function') {
                  delete descriptors[key].get
                }

                if ('set' in descriptors[key] && typeof descriptors[key].set !== 'function') {
                  delete descriptors[key].set
                }

                if (descriptors[key].get || descriptors[key].set) {
                  delete descriptors[key].writable
                  delete descriptors[key].value
                }
              }
            }

            Object.defineProperties(implementation.prototype, descriptors)
            Object.setPrototypeOf(implementation.prototype, nativeImplementation.prototype)
          }

          if (nativeImplementation !== null) {
            const nativeName = ['_', 'native', ...name.split('.')].join('_')
            natives[name] = nativeImplementation
            Object.defineProperty(globalThis, nativeName, {
              enumerable: false,
              configurable: false,
              value: nativeImplementation
            })
          }
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
    Notification,

    // pickers
    showDirectoryPicker,
    showOpenFilePicker,
    showSaveFilePicker,

    // events
    ApplicationURLEvent,

    // file
    File,
    FileSystemHandle,
    FileSystemFileHandle,
    FileSystemDirectoryHandle,
    FileSystemWritableFileStream
  })

  // navigator
  install({ geolocation, permissions }, globalThis.navigator, 'navigator')

  // WebAssembly
  install(WebAssembly, globalThis.WebAssembly, 'WebAssembly')

  applied = true
  // create <title> tag in document if it doesn't exist
  globalThis.document.title ||= ''
  // initial value
  globalThis.document.addEventListener('DOMContentLoaded', () => {
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

  return { natives, patches }
}

export default init()
