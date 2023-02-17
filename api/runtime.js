/**
 * @module Runtime
 *
 * Provides runtime-specific methods
 */

import ipc from './ipc.js'

export const currentWindow = globalThis?.window?.__args?.index ?? 0

/**
 *
 * @param {object} options - an options object
 * @param {number=} [options.window=currentWindow] - the window to send the message to
 * @param {string} options.event - the event to send
 * @param {(string|object)=} options.value - the value to send
 * @returns
 */
export async function send (options) {
  options.index = currentWindow
  options.window ??= -1

  if (typeof options.value !== 'string') {
    options.value = JSON.stringify(options.value)
  }

  return await ipc.send('send', {
    index: options.index,
    window: options.window,
    event: encodeURIComponent(options.event),
    value: encodeURIComponent(options.value)
  })
}

/**
 * @param {object} options
 * @returns {Promise<ipc.Result>}
 */
export async function openExternal (options) {
  return await ipc.postMessage(`ipc://external?value=${encodeURIComponent(options)}`)
}

export function reload () {
  ipc.postMessage('ipc://reload')
}

// eslint-disable-next-line
import * as exports from './runtime.js'
export default exports
