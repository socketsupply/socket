import * as ipc from '../ipc.js'
/**
 * @module DNS.promises
 *
 * This module enables name resolution. For example, use it to look up IP
 * addresses of host names. Although named for the Domain Name System (DNS),
 * it does not always use the DNS protocol for lookups. dns.lookup() uses the
 * operating system facilities to perform name resolution. It may not need to
 * perform any network communication. To perform name resolution the way other
 * applications on the same system do, use dns.lookup().
 */

import { rand64 } from '../util.js'

/**
 * @async
 * @see {@link https://nodejs.org/api/dns.html#dnspromiseslookuphostname-options}
 * @param {string} hostname - The host name to resolve.
 * @param {Object=} opts - An options object.
 * @param {number|string} [opts.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
 * @returns {Promise}
 */
export async function lookup (hostname, opts) {
  if (typeof hostname !== 'string') {
    const err = new TypeError(`The "hostname" argument must be of type string. Received type ${typeof hostname} (${hostname})`)
    err.code = 'ERR_INVALID_ARG_TYPE'
    throw err
  }

  if (typeof opts === 'number') {
    opts = { family: opts }
  }

  if (typeof opts !== 'object') {
    opts = {}
  }

  if (!opts.family) {
    opts.family = 4
  }

  const { err, data } = await ipc.send('dns.lookup', { ...opts, id: rand64(), hostname })

  if (err) {
    const e = new Error(`getaddrinfo EAI_AGAIN ${hostname}`)
    e.code = 'EAI_AGAIN'
    e.syscall = 'getaddrinfo'
    e.hostname = hostname
    // e.errno = -3008, // lib_uv constant?
    return e
  }

  return data
}
