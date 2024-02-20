import ipc from '../ipc.js'
import { parentPort } from '../worker_threads.js'
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

parentPort.onmessage = async ({ data: { id, method, args } }) => {
  if (method === 'spawn') {
    const command = args[0]
    const argv = args[1]
    const opts = args[2]

    const params = {
      args: [command, ...Array.from(argv ?? [])].join('\u0001'),
      id,
      cwd: opts?.cwd ?? ''
    }

    const { data, err } = await ipc.send('child_process.spawn', params)

    if (err) return propagateWorkerError(err)

    state.id = BigInt(data.id)
    state.pid = data.pid

    parentPort.postMessage({ method: 'state', args: [state] })

    globalThis.addEventListener('data', ({ detail }) => {
      const { err, data, source } = detail.params
      const buffer = detail.data

      if (err && err.id === state.id) {
        return propagateWorkerError(err)
      }

      if (!data || BigInt(data.id) !== state.id) return

      if (source === 'child_process.spawn' && data.source === 'stdout') {
        process.stdout.write(buffer)
      }

      if (source === 'child_process.spawn' && data.source === 'stderr') {
        process.stderr.write(buffer)
      }

      if (source === 'child_process.spawn' && data.status === 'close') {
        state.exitCode = data.code
        state.lifecycle = 'close'
        parentPort.postMessage({ method: 'state', args: [state] })
      }

      if (source === 'child_process.spawn' && data.status === 'exit') {
        state.exitCode = data.code
        state.lifecycle = 'exit'
        parentPort.postMessage({ method: 'state', args: [state] })
      }
    })
  }
}
