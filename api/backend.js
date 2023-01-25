/**
 * @module Backend
 *
 * Provides methods for the backend process management
 */

import ipc from './ipc.js'

/**
 * @param {object} opts - an options object
 * @param {boolean} [opts.force = false] - whether to force existing process to close
 * @return {Promise<ipc.Result>}
 */
export async function open (opts = {}) {
  opts.force ??= false
  return await ipc.send('process.open', opts)
}

/**
 * @return {Promise<ipc.Result>}
 */
export async function close () {
  return await ipc.send('process.kill')
}

/**
 * @return {Promise<ipc.Result>}
 */
export async function sendToProcess (opts) {
  return await ipc.send('process.write', opts)
}

// eslint-disable-next-line
import * as exports from './backend.js'
export default exports
