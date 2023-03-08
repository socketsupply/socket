/* global CustomEvent */
import ipc from '../ipc.js'

if (typeof globalThis?.addEventListener === 'function') {
  globalThis.addEventListener('message', onMessage)
}

export function onMessage (event) {
  const { data } = event

  if (typeof data?.__runtime_worker_ipc_result === 'object') {
    const resultData = data?.__runtime_worker_ipc_result || {}
    const message = ipc.Message.from(resultData.message)
    const { seq } = message
    const index = globalThis.__args.index
    const event = new CustomEvent(`resolve-${index}-${seq}`, {
      detail: resultData.result
    })

    globalThis.dispatchEvent(event)
  }
}

export * as default from './worker.js'
