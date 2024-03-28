import { Writable, Readable } from './stream.js'
import ipc from './ipc.js'

const textEncoder = new TextEncoder()
const textDecoder = new TextDecoder()

export function WriteStream (fd) {
  if (fd !== 1 && fd !== 2) {
    throw new TypeError('"fd" must be 1 or 2.')
  }

  const stream = new Writable({
    async write (data, cb) {
      if (data && typeof data === 'object') {
        data = textDecoder.decode(data)
      }

      for (const value of data.split('\n')) {
        if (fd === 1) {
          ipc.send('stdout', { resolve: false, value })
        } else if (fd === 2) {
          ipc.send('stderr', { resolve: false, value })
        }
      }

      cb(null)
    }
  })

  return stream
}

export function ReadStream (fd) {
  if (fd !== 0) {
    throw new TypeError('"fd" must be 0.')
  }

  const stream = new Readable()

  if (!globalThis.process?.versions?.node) {
    globalThis.addEventListener('process.stdin', function onData (event) {
      if (stream.destroyed) {
        return globalThis.removeEventListener('process.stdin', onData)
      }

      if (event.detail && typeof event.detail === 'object') {
        process.stdin.push(Buffer.from(JSON.stringify(event.detail)))
      } else if (event.detail) {
        process.stdin.push(Buffer.from(textEncoder.encode(event.detail)))
      }
    })
  }

  return stream
}

export function isatty (fd) {
  if (fd === 0) {
    return globalThis.__args?.argv?.includes?.('--stdin') !== true
  }

  if (fd === 1) {
    return true
  }

  if (fd === 2) {
    return true
  }

  return false
}

export default {
  WriteStream,
  ReadStream,
  isatty
}
