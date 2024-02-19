import ipc from '../ipc.js'
import { isMainThread, parentPort } from '../worker_threads.js'
import process from 'socket:process'

const state = {}

const propagateWorkerError = err => parentPort.postMessage({
  worker_threads: {
    error: {
      name: err.name,
      message: err.message,
      stack: err.stack,
      type: err.name
    }
  }
})

parentPort.onmessage = ({ data: { id, method, args } }) => {
  if (method === 'spawn') {
    const command = args[0]
    const argv = args[1]
    const opts = args[2]

    const params = {
      args: Array.from(argv ?? []).join('\u0001'),
      command,
      cwd: opts?.cwd
    }

    const { data, err } = await ipc.send('child_process.spawn', params)
    
    if (err) return propagateWorkerError(err)
    
    state.id = data.id
    state.pid = data.pid

    parentPort.postMessage({ method: 'state', args: [state] })
    
    globalThis.addEventListener('data', ({ detail }) => {
      const { err, data, source } = detail.params
      const buffer = detail.data

      if (err && err.id === state.id) {
        return propagateWokerError(err)
      }

      if (!data || BigInt(data.id) !== state.id) return

      if (source === 'child_process.spawn') {
        process.stdout.write(data)
      }
    })
  }
}
