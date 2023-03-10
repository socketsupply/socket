/* global CustomEvent */
import './init.js'
import ipc from '../ipc.js'
import hooks from '../hooks.js'
import console from '../console.js'

export default undefined // we MUST export _something_

let didImportSource = false

if (typeof globalThis.addEventListener === 'function') {
  globalThis.addEventListener('message', onWorkerMessage)
}

hooks.onReady(() => {
  globalThis.postMessage({ __runtime_worker_request_args: true })
})

let onmessage = null
Object.defineProperty(globalThis, 'onmessage', {
  get: () => onmessage,
  set: (value) => {
    onmessage = value
  }
})

export function onWorkerMessage (event) {
  const { data } = event

  if (typeof data?.__runtime_worker_args === 'object') {
    globalThis.__args = data.__runtime_worker_args

    if (!didImportSource) {
      didImportSource = true
      hooks.onReady(async () => {
        const url = new URL(import.meta.url)
        const source = url.searchParams.get('source')

        try {
          await import(source)
        } catch (err) {
          console.error(
            'RuntimeWorker: Failed to import worker:', err
          )
        }
      })
    }

    event.stopImmediatePropagation()
    return false
  } else if (typeof data?.__runtime_worker_ipc_result === 'object') {
    const resultData = data?.__runtime_worker_ipc_result || {}
    if (resultData.message && resultData.result) {
      const message = ipc.Message.from(resultData.message)
      const { seq } = message
      const index = globalThis.__args.index

      globalThis.dispatchEvent(new CustomEvent(`resolve-${index}-${seq}`, {
        detail: resultData.result
      }))
    }

    event.stopImmediatePropagation()
    return false
  } else if (typeof data?.__runtime_worker_event === 'object') {
    const { type, detail } = data?.__runtime_worker_event || {}
    if (type) {
      globalThis.dispatchEvent(new CustomEvent(type, { detail }))
    }
    event.stopImmediatePropagation()
    return false
  }

  if (typeof onmessage === 'function') {
    return onmessage(event)
  }
}
