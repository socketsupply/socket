import { Timeout, Immediate } from './timer.js'

export async function setTimeout (delay = 1, value = undefined, options = null) {
  return await new Promise((resolve, reject) => {
    const signal = options?.signal

    if (signal?.aborted) {
      return reject(new DOMException('This operation was aborted', 'AbortError'))
    }

    const timeout = Timeout.from(callback, delay)

    if (signal) {
      signal.addEventListener('abort', () => {
        timeout.close()
        reject(new DOMException('This operation was aborted', 'AbortError'))
      })
    }

    function callback () {
      resolve(value)
    }
  })
}

export async function * setInterval (delay = 1, value = undefined, options = null) {
  const signal = options?.signal

  while (true) {
    yield await new Promise((resolve, reject) => {
      const timeout = Timeout.from(callback, delay)

      if (signal?.aborted) {
        timeout.close()
        return reject(new DOMException('This operation was aborted', 'AbortError'))
      }

      function callback () {
        resolve(value)
      }
    })
  }
}

export async function setImmediate (value = undefined, options = null) {
  return await new Promise((resolve, reject) => {
    const signal = options?.signal

    if (signal?.aborted) {
      return reject(new DOMException('This operation was aborted', 'AbortError'))
    }

    const immediate = Immediate.from(callback, 0)

    if (signal) {
      signal.addEventListener('abort', () => {
        immediate.close()
        reject(new DOMException('This operation was aborted', 'AbortError'))
      })
    }

    function callback () {
      resolve(value)
    }
  })
}

export default {
  setImmediate,
  setInterval,
  setTimeout
}
