/**
 * @module SecureStore
 *
 * Native secret store
 */

export function get (account, service) {
  return await ipc.send('securestore.get', { account, service })
}

export function set (account, service, value) {
  return await ipc.write('securestore.set', {
    account,
    service,
    length: value.length
  }, value)
}

import * as exports from './secretstore.js'
export default exports
