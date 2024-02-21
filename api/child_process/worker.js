import { parentPort } from '../worker_threads.js'
import process from '../process.js'
import signal from '../signal.js'
import ipc from '../ipc.js'

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

if (process.stdin) {
  process.stdin.on('data', async (data) => {
    const { id } = state
    const result = await ipc.write('child_process.spawn', { id }, data)

    if (result.err) {
      propagateWorkerError(result.err)
    }
  })
}

parentPort.onmessage = async ({ data: { id, method, args } }) => {
  if (method === 'spawn') {
    const command = args[0]
    const argv = args[1]
    const opts = args[2]

    const params = {
      args: [command, ...Array.from(argv ?? [])].join('\u0001'),
      id,
      cwd: opts?.cwd ?? '',
      stdin: opts?.stdin !== false,
      stdout: opts?.stdout !== false,
      stderr: opts?.stderr !== false
    }

    const result = await ipc.send('child_process.spawn', params)

    if (result.err) {
      return propagateWorkerError(result.err)
    }

    state.id = BigInt(result.data.id)
    state.pid = result.data.pid
    state.spawnfile = command
    state.spawnargs = argv
    state.lifecycle = 'spawn'

    parentPort.postMessage({ method: 'state', args: [state] })

    globalThis.addEventListener('data', ({ detail }) => {
      const { err, data, source } = detail.params
      const buffer = detail.data

      if (err && err.id === state.id) {
        return propagateWorkerError(err)
      }

      if (!data || BigInt(data.id) !== state.id) return

      if (source === 'child_process.spawn' && data.source === 'stdout') {
        if (process.stdout) {
          process.stdout.write(buffer)
        }
      }

      if (source === 'child_process.spawn' && data.source === 'stderr') {
        if (process.stderr) {
          process.stderr.write(buffer)
        }
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

  if (method === 'kill') {
    const result = await ipc.send('child_process.kill', {
      id: state.id,
      signal: signal.getCode(args[0])
    })

    if (result.err) {
      return propagateWorkerError(result.err)
    }

    state.lifecycle = 'kill'
    parentPort.postMessage({ method: 'state', args: [state] })
  }
}
