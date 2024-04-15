/* global MutationObserver */
import './post-message.js'
import './error.js'

import { fetch, Headers, Request, Response } from '../fetch.js'
import { URL, URLPattern, URLSearchParams } from '../url.js'
import { ServiceWorker } from '../service-worker/instance.js'
import serviceWorker from './service-worker.js'
import SharedWorker from './shared-worker.js'
import Notification from '../notification.js'
import geolocation from './geolocation.js'
import permissions from './permissions.js'
import WebAssembly from './webassembly.js'
import { Buffer } from '../buffer.js'
import scheduler from './scheduler.js'
import Promise from './promise.js'
import symbols from './symbols.js'

import {
  ReadableStream,
  ReadableStreamBYOBReader,
  ReadableByteStreamController,
  ReadableStreamBYOBRequest,
  ReadableStreamDefaultController,
  ReadableStreamDefaultReader,

  WritableStream,
  WritableStreamDefaultController,
  WritableStreamDefaultWriter,

  TransformStream,
  TransformStreamDefaultController,

  ByteLengthQueuingStrategy,
  CountQueuingStrategy
} from './streams.js'

import {
  AsyncContext,
  AsyncResource,
  AsyncHook,
  AsyncLocalStorage,
  Deferred
} from '../async.js'

import {
  setTimeout,
  setInterval,
  setImmediate,
  clearTimeout,
  clearInterval,
  clearImmediate
} from './timers.js'

import {
  ApplicationURLEvent,
  MenuItemEvent,
  SignalEvent,
  HotKeyEvent
} from './events.js'

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
  if (applied || globalThis.self !== globalThis) {
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
            Object.defineProperty(
              target,
              actualName,
              Object.getOwnPropertyDescriptor(implementations, actualName)
            )
          } catch {}

          patches[actualName] = implementation
          if (
            (typeof nativeImplementation === 'function' && nativeImplementation.prototype) &&
            (typeof implementation === 'function' && implementation.prototype)
          ) {
            const nativeDescriptors = Object.getOwnPropertyDescriptors(nativeImplementation.prototype)
            const descriptors = Object.getOwnPropertyDescriptors(implementation.prototype)

            try {
              implementation[Symbol.species] = nativeImplementation
            } catch {}

            for (const key in nativeDescriptors) {
              const nativeDescriptor = nativeDescriptors[key]
              const descriptor = descriptors[key]

              if (key === 'constructor') {
                continue
              }

              if (descriptor) {
                if (nativeDescriptor.set && nativeDescriptor.get) {
                  descriptors[key] = { ...nativeDescriptor, ...descriptor }
                } else if (!nativeDescriptor.get && !nativeDescriptor.set) {
                  descriptors[key] = { ...nativeDescriptor, writable: true, configurable: true, ...descriptor }
                } else {
                  descriptors[key] = { writable: true, configurable: true, ...descriptor }
                }
              } else {
                descriptors[key] = { ...nativeDescriptor, writable: true, configurable: true }
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

  try {
    Symbol.dispose = symbols.dispose
    Symbol.serialize = symbols.serialize
  } catch {}

  if (
    typeof globalThis.webkitSpeechRecognition === 'function' &&
    typeof globalThis.SpeechRecognition !== 'function'
  ) {
    globalThis.SpeechRecognition = globalThis.webkitSpeechRecognition
  }

  if (globalThis.RUNTIME_WORKER_TYPE !== 'sharedWorker') {
    // globals
    install({
      // url
      URL,
      URLPattern,
      URLSearchParams,

      // Promise
      Promise,

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
      MenuItemEvent,
      SignalEvent,
      HotKeyEvent,

      // file
      File,
      FileSystemHandle,
      FileSystemFileHandle,
      FileSystemDirectoryHandle,
      FileSystemWritableFileStream,

      // buffer
      Buffer,

      // workers
      SharedWorker,
      ServiceWorker,

      // timers
      setTimeout,
      setInterval,
      setImmediate,
      clearTimeout,
      clearInterval,
      clearImmediate,

      // streams
      ReadableStream,
      ReadableStreamBYOBReader,
      ReadableByteStreamController,
      ReadableStreamBYOBRequest,
      ReadableStreamDefaultController,
      ReadableStreamDefaultReader,
      WritableStream,
      WritableStreamDefaultController,
      WritableStreamDefaultWriter,
      TransformStream,
      TransformStreamDefaultController,
      ByteLengthQueuingStrategy,
      CountQueuingStrategy,

      // async
      AsyncContext,
      AsyncResource,
      AsyncHook,
      AsyncLocalStorage,
      Deferred,

      // platform detection
      isSocketRuntime: true
    })
  }

  // globals
  install({
    // url
    URL,
    URLPattern,
    URLSearchParams,

    // Promise
    Promise,

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
    MenuItemEvent,
    SignalEvent,
    HotKeyEvent,

    // file
    File,
    FileSystemHandle,
    FileSystemFileHandle,
    FileSystemDirectoryHandle,
    FileSystemWritableFileStream,

    // buffer
    Buffer,

    // workers
    SharedWorker,
    ServiceWorker,

    // timers
    setTimeout,
    setInterval,
    setImmediate,
    clearTimeout,
    clearInterval,
    clearImmediate,

    // streams
    ReadableStream,
    ReadableStreamBYOBReader,
    ReadableByteStreamController,
    ReadableStreamBYOBRequest,
    ReadableStreamDefaultController,
    ReadableStreamDefaultReader,
    WritableStream,
    WritableStreamDefaultController,
    WritableStreamDefaultWriter,
    TransformStream,
    TransformStreamDefaultController,
    ByteLengthQueuingStrategy,
    CountQueuingStrategy,

    // async
    AsyncContext,
    AsyncResource,
    AsyncHook,
    AsyncLocalStorage,
    Deferred,

    // platform detection
    isSocketRuntime: true
  })

  if (globalThis.scheduler) {
    install(scheduler, globalThis.scheduler)
  }

  if (globalThis.navigator) {
    if (globalThis.window) {
      install({ geolocation }, globalThis.navigator, 'geolocation')
    }

    install({ permissions, serviceWorker }, globalThis.navigator, 'navigator')

    // manually install 'navigator.serviceWorker' accessors from prototype
    Object.defineProperties(
      globalThis.navigator.serviceWorker,
      Object.getOwnPropertyDescriptors(Object.getPrototypeOf(serviceWorker))
    )

    // manually initialize `ServiceWorkerContainer` instance with the
    // runtime implementations
    if (typeof serviceWorker.init === 'function') {
      serviceWorker.init.call(globalThis.navigator.serviceWorker)
      delete serviceWorker.init
    }

    // TODO(@jwerle): handle 'popstate' for service workers
    // globalThis.addEventListener('popstate', (event) => { })
  }

  // WebAssembly
  install(WebAssembly, globalThis.WebAssembly, 'WebAssembly')

  applied = true

  if (!Error.captureStackTrace) {
    Error.captureStackTrace = function () {}
  }

  if (globalThis.document && globalThis.top === globalThis) {
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

    // globalThis.document is unconfigurable property so we need to use MutationObserver here
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

  return { natives, patches }
}

export default init()
