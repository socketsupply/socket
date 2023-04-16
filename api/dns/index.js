/**
 * @module DNS
 *
 * This module enables name resolution. For example, use it to look up IP
 * addresses of host names. Although named for the Domain Name System (DNS),
 * it does not always use the DNS protocol for lookups. dns.lookup() uses the
 * operating system facilities to perform name resolution. It may not need to
 * perform any network communication. To perform name resolution the way other
 * applications on the same system do, use dns.lookup().
 *
 * Example usage:
 * ```js
 * import { lookup } from 'socket:dns'
 * ```
 */

import { isFunction } from '../util.js'
import * as promises from './promises.js'
import diagnostics from '../diagnostics.js'
import { rand64 } from '../crypto.js'
import ipc from '../ipc.js'

import * as exports from './index.js'

const dc = diagnostics.channels.group('dns', [
  'lookup.start',
  'lookup.end',
  'lookup'
])

/**
 * Resolves a host name (e.g. `example.org`) into the first found A (IPv4) or
 * AAAA (IPv6) record. All option properties are optional. If options is an
 * integer, then it must be 4 or 6 – if options is 0 or not provided, then IPv4
 * and IPv6 addresses are both returned if found.
 *
 * From the node.js website...
 *
 * > With the all option set to true, the arguments for callback change to (err,
 * addresses), with addresses being an array of objects with the properties
 * address and family.
 *
 * > On error, err is an Error object, where err.code is the error code. Keep in
 * mind that err.code will be set to 'ENOTFOUND' not only when the host name does
 * not exist but also when the lookup fails in other ways such as no available
 * file descriptors. dns.lookup() does not necessarily have anything to do with
 * the DNS protocol. The implementation uses an operating system facility that
 * can associate names with addresses and vice versa. This implementation can
 * have subtle but important consequences on the behavior of any Node.js program.
 * Please take some time to consult the Implementation considerations section
 * before using dns.lookup().
 *
 * @see {@link https://nodejs.org/api/dns.html#dns_dns_lookup_hostname_options_callback}
 * @param {string} hostname - The host name to resolve.
 * @param {Object=} opts - An options object.
 * @param {number|string} [opts.family=0] - The record family. Must be 4, 6, or 0. For backward compatibility reasons,'IPv4' and 'IPv6' are interpreted as 4 and 6 respectively. The value 0 indicates that IPv4 and IPv6 addresses are both returned. Default: 0.
 * @param {function} cb - The function to call after the method is complete.
 * @returns {void}
 */
export function lookup (hostname, opts, cb) {
  if (typeof hostname !== 'string') {
    const err = new TypeError(`The "hostname" argument must be of type string. Received type ${typeof hostname} (${hostname})`)
    err.code = 'ERR_INVALID_ARG_TYPE'
    throw err
  }

  if (typeof opts === 'function') {
    cb = opts
  }

  if (typeof cb !== 'function') {
    const err = new TypeError(`The "callback" argument must be of type function. Received type ${typeof cb} undefined`)
    err.code = 'ERR_INVALID_ARG_TYPE'
    throw err
  }

  if (typeof opts === 'number') {
    opts = { family: opts }
  }

  if (typeof opts !== 'object') {
    opts = {}
  }

  dc.channel('lookup.start').publish({ hostname, family: opts.family, sync: true })
  const { err, data } = ipc.sendSync('dns.lookup', { ...opts, id: rand64(), hostname })

  if (err) {
    const e = new Error(`getaddrinfo EAI_AGAIN ${hostname}`)
    e.code = 'EAI_AGAIN'
    e.syscall = 'getaddrinfo'
    e.hostname = hostname
    e.cause = err
    // e.errno = -3008, // lib_uv constant?
    cb(e, null, null)
    return
  }

  dc.channel('lookup.end').publish({ hostname, family: opts.family, sync: true })
  dc.channel('lookup').publish({ hostname, family: opts.family, sync: true })
  cb(null, data?.address ?? null, data?.family ?? null)
}

export {
  promises
}

export default exports

for (const key in exports) {
  const value = exports[key]
  if (key in promises && isFunction(value) && isFunction(promises[key])) {
    value[Symbol.for('nodejs.util.promisify.custom')] = promises[key]
  }
}
